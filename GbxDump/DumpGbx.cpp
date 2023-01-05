////////////////////////////////////////////////////////////////////////////////////////////////
// DumpGbx.cpp - Copyright (c) 2010-2022 by Electron.
//
// Licensed under the EUPL, Version 1.2 or - as soon they will be approved by
// the European Commission - subsequent versions of the EUPL (the "Licence");
// You may not use this work except in compliance with the Licence.
// You may obtain a copy of the Licence at:
//
// https://joinup.ec.europa.eu/software/page/eupl
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Licence is distributed on an "AS IS" basis,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the Licence for the specific language governing permissions and
// limitations under the Licence.
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Based on information from https://wiki.xaseco.org/wiki/GBX
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "file.h"
#include "classid.h"
#include "archive.h"
#include "dumpgbx.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
//
#define POS_NUMBER_CHUNKS         17
#define POS_HEADER_CHUNKS         21

// Macros
#define IS_GBX_TEXT(ach)          ((ach)[0] == 'T')
#define IS_GBX_BINARY(ach)        ((ach)[0] == 'B')
#define IS_REF_COMPRESSED(ach)    ((ach)[1] == 'C')
#define IS_REF_UNCOMPRESSED(ach)  ((ach)[1] == 'U')
#define IS_BODY_COMPRESSED(ach)   ((ach)[2] == 'C')
#define IS_BODY_UNCOMPRESSED(ach) ((ach)[2] == 'U')

#define GET_CHUNK_OFFSET(num)     (((num) * 8) + POS_HEADER_CHUNKS)

////////////////////////////////////////////////////////////////////////////////////////////////
// Data Types
//
typedef enum _BASECLASS
{
	eOther = 0,
	eChallenge,
	eReplay,
	eCollector,
	eCollection,
	eProfile,
	eSkin,
	ePlug,
	eHms
} BASECLASS;

typedef struct _CHUNK
{
	DWORD dwId;
	DWORD dwSize;
	DWORD dwOffset;
} CHUNK, *PCHUNK, *LPCHUNK;

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module
//
BASECLASS GetBaseClass(HWND hwndCtl, DWORD dwClassId);
DWORD ReadRefTable(HWND hwndCtl, HANDLE hFile, WORD wVersion, PBYTE achStorageSettings);
BOOL ReadSubFolders(HWND hwndCtl, HANDLE hFile, PDWORD pdwIndex, LPCSTR lpszFolder, BOOL bIsText);
BOOL ReadSkin(HWND hwndCtl, HANDLE hFile);

BOOL DumpChallenge(HWND hwndCtl, HANDLE hFile, LPSTR lpszUid, LPSTR lpszEnvi);
BOOL DumpReplay(HWND hwndCtl, HANDLE hFile, LPSTR lpszUid, LPSTR lpszEnvi);
BOOL DumpCollector(HWND hwndCtl, HANDLE hFile);
BOOL DumpSkin(HWND hwndCtl, HANDLE hFile);
BOOL DumpProfile(HWND hwndCtl, HANDLE hFile);
BOOL DumpCollection(HWND hwndCtl, HANDLE hFile);
BOOL DumpPlug(HWND hwndCtl, HANDLE hFile);
BOOL DumpHms(HWND hwndCtl, HANDLE hFile);
BOOL DumpOther(HWND hwndCtl, HANDLE hFile);

BOOL ChallengeTmDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkTmDesc);
BOOL ChallengeCommonChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkCommon, LPSTR lpszUid, LPSTR lpszEnvi);
BOOL ChallengeVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkVersion);
BOOL ChallengeThumbnailChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkThumbnail);
BOOL ChallengeVskDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkVskDesc);

BOOL ReplayVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkVersion, LPSTR lpszUid, LPSTR lpszEnvi);

BOOL ChallengeReplayCommunityChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkCommunity);
BOOL ChallengeReplayAuthorChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkAuthor);

BOOL CollectorDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkDesc);
BOOL CollectorIconChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkIcon);
BOOL CollectorTimeChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkTime);
BOOL CollectorSkinChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkSkin);

BOOL ObjectInfoTypeChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkType);
BOOL ObjectInfoVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkVersion);

BOOL DecorationMoodChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkMood);
BOOL DecorationUnknownChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkUnknown);

BOOL CollectionOldDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkOldDesc);
BOOL CollectionDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkDesc);
BOOL CollectionFoldersChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkFolders);
BOOL CollectionMenuIconsChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkMenuIcons);

BOOL PlugVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkVersion);
BOOL HmsVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkVersion);
BOOL GameSkinChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkGameSkin);
BOOL ProfileChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkProfile);
BOOL FolderDepChunk(HWND hwndCtl, HANDLE hFile, PCHUNK chunkFolder);

////////////////////////////////////////////////////////////////////////////////////////////////
// String Constants
//
const TCHAR g_chNil        = TEXT('\0');
const TCHAR g_szAsterisk[] = TEXT("*");
const TCHAR g_szCRLF[]     = TEXT("\r\n");
const TCHAR g_szYes[]      = TEXT("Yes");
const TCHAR g_szNo[]       = TEXT("No");
const TCHAR g_szTrue[]     = TEXT("True");
const TCHAR g_szFalse[]    = TEXT("False");
const TCHAR g_szUnknown[]  = TEXT("Unknown");
const TCHAR g_szFolder[]   = TEXT("Folder:\t\t");
const TCHAR g_szVersion[]  = TEXT("Chunk Version:\t%d");
const TCHAR g_szChunk[]    = TEXT("Chunk %u:\t[%08X] (%u bytes)\r\n");

// Environment Names
const CHAR g_szTm2Envis[] = "Canyon,Valley,Stadium,Lagoon,Arena";
const CHAR g_szTm1Envis[] = "Stadium,Coast,Bay,Island,Desert,Rally,Snow,Alpine,Speed";
const CHAR g_szMpEnvis[]  = "Canyon,Valley,Stadium,Lagoon,Arena,Storm,Cryo,Meteor,Paris,Gothic,Future,Galaxy,History,Society";

////////////////////////////////////////////////////////////////////////////////////////////////
// DumpGbx is called by DumpFile from GbxDump.cpp

BOOL DumpGbx(HWND hwndCtl, HANDLE hFile, LPSTR lpszUid, LPSTR lpszEnvi)
{
	BOOL bRet = FALSE;
	TCHAR szOutput[OUTPUT_LEN];

	if (hwndCtl == NULL || hFile == NULL)
		return FALSE;

	// Skip the file signature (already checked in DumpFile())
	if (!FileSeekBegin(hFile, 3))
		return FALSE;

	// Read GBX file version
	WORD wVersion = 0;
	if (!ReadNat16(hFile, &wVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("File Version:\t%d"), wVersion);
	if (wVersion > 6) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	if (wVersion > 6)
	{ // Unsupported or corrupted file version
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_VERSION : IDP_ENG_ERR_VERSION);
		OutputText(hwndCtl, g_szSep1);
	}

	if (wVersion < 3)
	{ // From here no further support for GBX files below version 3
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_UNAVAIL : IDP_ENG_ERR_UNAVAIL);
		return TRUE;
	}

	// Read GBX storage settings
	// For version 3 GBX files, the storage settings are only three bytes long
	BYTE achStorageSettings[4] = {'X', 'X', 'X', 'X'};
	if (!ReadData(hFile, (LPVOID)&achStorageSettings, (wVersion >= 4) ? 4 : 3))
		return FALSE;

#ifdef _DEBUG
	OutputText(hwndCtl, TEXT("File Settings:\t"));
	OutputTextFmt(hwndCtl, szOutput, TEXT("%hc%hc%hc"), achStorageSettings[0],
		achStorageSettings[1], achStorageSettings[2]);
	if (wVersion >= 4)
		OutputTextFmt(hwndCtl, szOutput, TEXT("%hc"), achStorageSettings[3]);
	OutputText(hwndCtl, g_szCRLF);
#endif

	OutputTextFmt(hwndCtl, szOutput, TEXT("File Format:\t%s\r\n"),
		IS_GBX_TEXT(achStorageSettings) ? TEXT("Text") : TEXT("Binary"));
	// The RefTable block should always be uncompressed
	OutputTextFmt(hwndCtl, szOutput, TEXT("Ref Table:\t%s\r\n"),
		IS_REF_COMPRESSED(achStorageSettings) ? TEXT("Compressed*") : TEXT("Uncompressed"));
	OutputTextFmt(hwndCtl, szOutput, TEXT("File Body:\t%s\r\n"),
		IS_BODY_UNCOMPRESSED(achStorageSettings) ? TEXT("Uncompressed") : TEXT("Compressed"));

	// Read Class ID
	DWORD dwClassId = 0;
	if (!ReadMask(hFile, &dwClassId, IS_GBX_TEXT(achStorageSettings)))
		return FALSE;

	// Output Class ID and determine the base class from the ID.
	OutputTextFmt(hwndCtl, szOutput, TEXT("Class ID:\t%08X"), dwClassId);
	BASECLASS eBaseClass = GetBaseClass(hwndCtl, dwClassId);
	OutputText(hwndCtl, g_szCRLF);

	// From version 6 onwards, user data is supported in the file header
	DWORD dwUserDataSize = 0;
	if (wVersion >= 6)
	{
		// Read the Header User Data block size
		if (!ReadNat32(hFile, &dwUserDataSize, IS_GBX_TEXT(achStorageSettings)) || dwUserDataSize > 0x200000)
			return FALSE;

		if (FormatByteSize(dwUserDataSize, szOutput, _countof(szOutput)))
		{
			OutputText(hwndCtl, TEXT("User Data:\t"));
			OutputText(hwndCtl, szOutput);
			OutputText(hwndCtl, g_szCRLF);
		}
	}

	// Jump to number of references (skip Header User Data)
	if (!FileSeekCurrent(hFile, dwUserDataSize))
		return FALSE;

	// Read the number of references
	DWORD dwNumEntries = 0;
	if (!ReadNat32(hFile, &dwNumEntries, IS_GBX_TEXT(achStorageSettings)) || dwNumEntries > 50000)
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Number Refs:\t%d\r\n"), dwNumEntries);

	// Read external references and determine the size of the body in case of a compressed body
	DWORD dwExtEntries = ReadRefTable(hwndCtl, hFile, wVersion, achStorageSettings);
	if (dwExtEntries != (DWORD)-1 && IS_BODY_COMPRESSED(achStorageSettings))
	{
		DWORD dwCompressedSize = 0;
		DWORD dwUncompressedSize = 0;

		// Read size of the uncompressed body
		if (!ReadNat32(hFile, &dwUncompressedSize, IS_GBX_TEXT(achStorageSettings)))
			dwUncompressedSize = 0;
		// Read size of the compressed body
		if (!ReadNat32(hFile, &dwCompressedSize, IS_GBX_TEXT(achStorageSettings)))
			dwCompressedSize = 0;

		if (dwCompressedSize > 0 && FormatByteSize(dwCompressedSize, szOutput, _countof(szOutput)))
		{ // Output size of the compressed body
			if (dwExtEntries > 0) OutputText(hwndCtl, g_szSep1);
			OutputText(hwndCtl, TEXT("Body Size:\t"));
			OutputText(hwndCtl, szOutput);

			if (dwUncompressedSize > 0 && FormatByteSize(dwUncompressedSize, szOutput, _countof(szOutput)))
			{ // Output uncompressed body size
				OutputText(hwndCtl, TEXT(" ("));
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT(" uncompressed)"));
			}

			OutputText(hwndCtl, g_szCRLF);
		}
	}

	if (dwUserDataSize == 0)
	{
		// No user data available (not supported or empty)
		if (wVersion < 6)
			OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_UNAVAIL : IDP_ENG_ERR_UNAVAIL);
		else
			OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_EMPTY : IDP_ENG_ERR_EMPTY);
		return TRUE;
	}

	// From this point on no further support for GBX files in text format
	if (IS_GBX_TEXT(achStorageSettings))
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_TEXT : IDP_ENG_ERR_TEXT);
		return TRUE;
	}

	// Dump Header User Data
	switch (eBaseClass)
	{
		case eChallenge:
			bRet = DumpChallenge(hwndCtl, hFile, lpszUid, lpszEnvi);
			break;
		case eReplay:
			bRet = DumpReplay(hwndCtl, hFile, lpszUid, lpszEnvi);
			break;
		case eCollector:
			bRet = DumpCollector(hwndCtl, hFile);
			break;
		case eSkin:
			bRet = DumpSkin(hwndCtl, hFile);
			break;
		case eProfile:
			bRet = DumpProfile(hwndCtl, hFile);
			break;
		case eCollection:
			bRet = DumpCollection(hwndCtl, hFile);
			break;
		case ePlug:
			bRet = DumpPlug(hwndCtl, hFile);
			break;
		case eHms:
			bRet = DumpHms(hwndCtl, hFile);
			break;
		case eOther:
		default:
			bRet = DumpOther(hwndCtl, hFile);
	}

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BASECLASS GetBaseClass(HWND hwndCtl, DWORD dwClassId)
{
	BASECLASS eBaseClass = eOther;

	switch (dwClassId)
	{
		case CLSID_CHALLENGE_TMF:
		case CLSID_CHALLENGE_TM:
		case CLSID_CHALLENGE_VSK:
			eBaseClass = eChallenge;
			OutputText(hwndCtl, TEXT(" (Challenge)"));
			break;

		case CLSID_REPLAYRECORD_TM:
		case CLSID_REPLAYRECORD_TMF:
		case CLSID_REPLAYRECORD_VSK:
		case CLSID_AUTOSAVE_TM:
			eBaseClass = eReplay;
			OutputText(hwndCtl, TEXT(" (ReplayRecord)"));
			break;

		case CLSID_ITEMMODEL_MP:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (ItemModel)"));
			break;

		case CLSID_OBJECTINFO_TMF:
		case CLSID_OBJECTINFO_TM:
		case CLSID_OBJECTINFO_VSK:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (ObjectInfo)"));
			break;

		case CLSID_CARDEVENT_MP:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (CardEventInfo)"));
			break;

		case CLSID_MACROBLOCK_MP:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (MacroBlockInfo)"));
			break;

		case CLSID_MACRODECALS_MP:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (MacroDecals)"));
			break;

		case CLSID_VEHICLE_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (CollectorVehicle)"));
			break;

		case CLSID_DECORATION_TMF:
		case CLSID_DECORATION_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (Decoration)"));
			break;

		case CLSID_BLOCKFLAT_TNF:
		case CLSID_BLOCKFLAT_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoFlat)"));
			break;

		case CLSID_BLOCKFRONTIER_TNF:
		case CLSID_BLOCKFRONTIER_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoFrontier)"));
			break;

		case CLSID_BLOCKCLASSIC_TNF:
		case CLSID_BLOCKCLASSIC_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoClassic)"));
			break;

		case CLSID_BLOCKROAD_TNF:
		case CLSID_BLOCKROAD_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoRoad)"));
			break;

		case CLSID_BLOCKCLIP_TNF:
		case CLSID_BLOCKCLIP_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoClip)"));
			break;

		case CLSID_BLOCKSLOPE_TNF:
		case CLSID_BLOCKSLOPE_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoSlope)"));
			break;

		case CLSID_BLOCKPYLON_TNF:
		case CLSID_BLOCKPYLON_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoPylon)"));
			break;

		case CLSID_BLOCKRECTASYM_TNF:
		case CLSID_BLOCKRECTASYM_TM:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoRectAsym)"));
			break;

		case CLSID_BLOCKTRANSITION_MP:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (BlockInfoTransition)"));
			break;

		case CLSID_BLOCKMODEL_MP:
			eBaseClass = eCollector;
			OutputText(hwndCtl, TEXT(" (CustomBlockModel)"));
			break;

		case CLSID_SKIN_TMF:
		case CLSID_SKIN_MP:
			eBaseClass = eSkin;
			OutputText(hwndCtl, TEXT(" (Skin)"));
			break;

		case CLSID_PLAYERPROFILE_TMF:
		case CLSID_PLAYERPROFILE_TM:
		case CLSID_PLAYERPROFILE_VSK:
			eBaseClass = eProfile;
			OutputText(hwndCtl, TEXT(" (PlayerProfile)"));
			break;

		case CLSID_USERPROFILE_MP:
			OutputText(hwndCtl, TEXT(" (UserProfile)"));
			break;

		case CLSID_COLLECTION_TMF:
		case CLSID_COLLECTION_TM:
		case CLSID_COLLECTION_VSK:
			eBaseClass = eCollection;
			OutputText(hwndCtl, TEXT(" (Collection)"));
			break;

		case CLSID_ANIMFILE_MP:
			eBaseClass = ePlug;
			OutputText(hwndCtl, TEXT(" (AnimFile)"));
			break;

		case CLSID_SOLID2MODEL_MP:
			eBaseClass = ePlug;
			OutputText(hwndCtl, TEXT(" (Solid2Model)"));
			break;

		case CLSID_ACTIONMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ActionModel)"));
			break;

		case CLSID_MENUMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModuleMenuModel)"));
			break;

		case CLSID_COMMONMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModuleModelCommon)"));
			break;

		case CLSID_INVENTORYMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModulePlaygroundInventoryModel)"));
			break;

		case CLSID_SCORESMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModulePlaygroundScoresTableModel)"));
			break;

		case CLSID_STOREMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModulePlaygroundStoreModel)"));
			break;

		case CLSID_HUDMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModulePlaygroundHudModel)"));
			break;

		case CLSID_MENUPAGEMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModuleMenuPageModel)"));
			break;

		case CLSID_EDITORMODEL_MP:
			OutputText(hwndCtl, TEXT(" (EditorModel)"));
			break;

		case CLSID_VEHICLEMODEL_MP:
			OutputText(hwndCtl, TEXT(" (VehicleModel)"));
			break;

		case CLSID_CHRONOMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModulePlaygroundChronoModel)"));
			break;

		case CLSID_METERMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModulePlaygroundSpeedMeterModel)"));
			break;

		case CLSID_PLAYERMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModulePlaygroundPlayerStateModel)"));
			break;

		case CLSID_TEAMMODEL_MP:
			OutputText(hwndCtl, TEXT(" (ModulePlaygroundTeamStateModel)"));
			break;

		case CLSID_PIXELARTMODEL_MP:
			OutputText(hwndCtl, TEXT(" (PixelArtModel)"));
			break;

		case CLSID_BLOCKITEM_MP:
			OutputText(hwndCtl, TEXT(" (BlockItem)"));
			break;

		case CLSID_CRYSTAL_TM:
			OutputText(hwndCtl, TEXT(" (Crystal)"));
			break;

		case CLSID_SOLID_TM:
			OutputText(hwndCtl, TEXT(" (Solid)"));
			break;

		case CLSID_MATERIAL_TMF:
			OutputText(hwndCtl, TEXT(" (Material)"));
			break;

		case CLSID_MATUSERINST_MP:
			OutputText(hwndCtl, TEXT(" (MaterialUserInst)"));
			break;

		case CLSID_SURFACE_TM:
			OutputText(hwndCtl, TEXT(" (Surface)"));
			break;

		case CLSID_BITMAP_TM:
			OutputText(hwndCtl, TEXT(" (Bitmap)"));
			break;

		case CLSID_FONTBITMAP_TM:
			OutputText(hwndCtl, TEXT(" (FontBitmap)"));
			break;

		case CLSID_GHOST_TMF:
		case CLSID_GHOST_TM:
			OutputText(hwndCtl, TEXT(" (Ghost)"));
			break;

		case CLSID_MEDIACLIP_TMF:
		case CLSID_MEDIACLIP_TM:
			OutputText(hwndCtl, TEXT(" (MediaClip)"));
			break;

		case CLSID_CAMPAIGN_TMF:
		case CLSID_CAMPAIGN_TM:
			OutputText(hwndCtl, TEXT(" (Campaign)"));
			break;

		case CLSID_USERFILELIST_MP:
			OutputText(hwndCtl, TEXT(" (UserFileList)"));
			break;

		case CLSID_SYSTEMCONFIG_TM:
			OutputText(hwndCtl, TEXT(" (Config)"));
			break;

		case CLSID_DX9DEVICECAPS_TM:
			OutputText(hwndCtl, TEXT(" (Dx9DeviceCaps)"));
			break;

		case CLSID_REFBUFFER_TM:
			OutputText(hwndCtl, TEXT(" (RefBuffer)"));
			break;

		case CLSID_SHADERAPPLY_TM:
			OutputText(hwndCtl, TEXT(" (ShaderApply)"));
			break;

		case CLSID_SHADERLAYERUV_TM:
			OutputText(hwndCtl, TEXT(" (ShaderLayerUV)"));
			break;

		case CLSID_GPUCOMPILECACHE_MP:
			OutputText(hwndCtl, TEXT(" (GpuCompileCache)"));
			break;

		case CLSID_LIGHTMAP_TM:
			eBaseClass = eHms;
			OutputText(hwndCtl, TEXT(" (LightMap)"));
			break;

		case CLSID_LIGHTMAPCACHE_TM:
			OutputText(hwndCtl, TEXT(" (LightMapCache)"));
			break;

		case CLSID_TRAITSPERSIST_MP:
			OutputText(hwndCtl, TEXT(" (TraitsPersistent)"));
			break;

		case CLSID_INPUTREPLAY_MP:
			OutputText(hwndCtl, TEXT(" (Replay)"));
			break;

		case CLSID_PLAYERSCORE_TMF:
		case CLSID_PLAYERSCORE_TM:
			OutputText(hwndCtl, TEXT(" (PlayerScore)"));
			break;

		case CLSID_LADDERSCORES_TMF:
			OutputText(hwndCtl, TEXT(" (LadderScores)"));
			break;

		case CLSID_LEAGUEMANAGER_TMF:
			OutputText(hwndCtl, TEXT(" (LeagueManager)"));
			break;

		case CLSID_SCORESMANAGER_TMF:
			OutputText(hwndCtl, TEXT(" (CampaignsScoresManager)"));
			break;

		case CLSID_REGATTA_VSK:
			OutputText(hwndCtl, TEXT(" (Regatta)"));
			break;

		case CLSID_ACMODEL_VSK:
			OutputText(hwndCtl, TEXT(" (ACModel)"));
			break;

		case CLSID_TOYBOAT_VSK:
			OutputText(hwndCtl, TEXT(" (ToyBoat)"));
			break;

		case CLSID_BOATPARAM_VSK:
			OutputText(hwndCtl, TEXT(" (BoatParam)"));
			break;
	}

	return eBaseClass;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpChallenge(HWND hwndCtl, HANDLE hFile, LPSTR lpszUid, LPSTR lpszEnvi)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkTmDesc = {0};
	CHUNK chunkCommon = {0};
	CHUNK chunkVersion = {0};
	CHUNK chunkCommunity = {0};
	CHUNK chunkThumbnail = {0};
	CHUNK chunkAuthor = {0};
	CHUNK chunkVskDesc = {0};
	TCHAR szOutput[OUTPUT_LEN];

	// Jump to number of chunks in the header
	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	// Number of Chunks
	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF) // sanity check
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	// Determine chunk sizes and positions
	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x03043002: // (TM)
			case 0x24003002: // (TM)
				chunkTmDesc.dwId = dwChunkId;
				chunkTmDesc.dwSize = dwChunkSize;
				chunkTmDesc.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03043003: // (TM)
			case 0x24003003: // (VSK, TM)
				chunkCommon.dwId = dwChunkId;
				chunkCommon.dwSize = dwChunkSize;
				chunkCommon.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03043004: // (TM)
			case 0x24003004: // (VSK, TM)
				chunkVersion.dwId = dwChunkId;
				chunkVersion.dwSize = dwChunkSize;
				chunkVersion.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03043005: // (TM)
			case 0x24003005: // (VSK, TM)
				chunkCommunity.dwId = dwChunkId;
				chunkCommunity.dwSize = dwChunkSize;
				chunkCommunity.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03043007: // (TM)
			case 0x24003007: // (VSK, TM)
				chunkThumbnail.dwId = dwChunkId;
				chunkThumbnail.dwSize = dwChunkSize;
				chunkThumbnail.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03043008: // (MP)
				chunkAuthor.dwId = dwChunkId;
				chunkAuthor.dwSize = dwChunkSize;
				chunkAuthor.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x21080001: // (VSK)
				chunkVskDesc.dwId = dwChunkId;
				chunkVskDesc.dwSize = dwChunkSize;
				chunkVskDesc.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// TMDesc chunk
	if (chunkTmDesc.dwSize > 0)
		bSuccess &= ChallengeTmDescChunk(hwndCtl, hFile, &chunkTmDesc);

	// Common chunk
	if (chunkCommon.dwSize > 0)
		bSuccess &= ChallengeCommonChunk(hwndCtl, hFile, &chunkCommon, lpszUid, lpszEnvi);

	// Version chunk
	if (chunkVersion.dwSize > 0)
		bSuccess &= ChallengeVersionChunk(hwndCtl, hFile, &chunkVersion);

	// Community chunk
	if (chunkCommunity.dwSize > 0)
		bSuccess &= ChallengeReplayCommunityChunk(hwndCtl, hFile, &chunkCommunity);

	// Thumbnail chunk
	if (chunkThumbnail.dwSize > 0)
		bSuccess &= ChallengeThumbnailChunk(hwndCtl, hFile, &chunkThumbnail);

	// Author chunk
	if (chunkAuthor.dwSize > 0)
		bSuccess &= ChallengeReplayAuthorChunk(hwndCtl, hFile, &chunkAuthor);

	// VSKDesc chunk
	if (chunkVskDesc.dwSize > 0)
		bSuccess &= ChallengeVskDescChunk(hwndCtl, hFile, &chunkVskDesc);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpReplay(HWND hwndCtl, HANDLE hFile, LPSTR lpszUid, LPSTR lpszEnvi)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkVersion = {0};
	CHUNK chunkCommunity = {0};
	CHUNK chunkAuthor = {0};
	TCHAR szOutput[OUTPUT_LEN];

	// Jump to number of chunks in the header
	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	// Number of chunks
	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF) // sanity check
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	// Determine chunk sizes and positions
	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x03093000: // (TM)
			case 0x2403F000: // (VSK, TM)
				chunkVersion.dwId = dwChunkId;
				chunkVersion.dwSize = dwChunkSize;
				chunkVersion.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03093001: // (TM)
			case 0x2403F001: // (VSK, TM)
				chunkCommunity.dwId = dwChunkId;
				chunkCommunity.dwSize = dwChunkSize;
				chunkCommunity.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03093002: // (MP)
				chunkAuthor.dwId = dwChunkId;
				chunkAuthor.dwSize = dwChunkSize;
				chunkAuthor.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// Version chunk
	if (chunkVersion.dwSize > 0)
		bSuccess &= ReplayVersionChunk(hwndCtl, hFile, &chunkVersion, lpszUid, lpszEnvi);

	// Community chunk
	if (chunkCommunity.dwSize > 0)
		bSuccess &= ChallengeReplayCommunityChunk(hwndCtl, hFile, &chunkCommunity);

	// Author chunk
	if (chunkAuthor.dwSize > 0)
		bSuccess &= ChallengeReplayAuthorChunk(hwndCtl, hFile, &chunkAuthor);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpCollector(HWND hwndCtl, HANDLE hFile)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkFolder = {0};
	CHUNK chunkDesc = {0};
	CHUNK chunkIcon = {0};
	CHUNK chunkTime = {0};
	CHUNK chunkSkin = {0};
	CHUNK chunkType = {0};
	CHUNK chunkGameSkin = {0};
	CHUNK chunkMood = {0};
	CHUNK chunkVersion = {0};
	CHUNK chunkUnknown = {0};
	TCHAR szOutput[OUTPUT_LEN];

	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF)
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x01001000:
				chunkFolder.dwId = dwChunkId;
				chunkFolder.dwSize = dwChunkSize;
				chunkFolder.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x0301A003:
			case 0x2400A003:
			case 0x2E001003:
				chunkDesc.dwId = dwChunkId;
				chunkDesc.dwSize = dwChunkSize;
				chunkDesc.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x0301A004:
			case 0x2400A004:
			case 0x2E001004:
				chunkIcon.dwId = dwChunkId;
				chunkIcon.dwSize = dwChunkSize;
				chunkIcon.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x0301A006:
			case 0x2E001006:
				chunkTime.dwId = dwChunkId;
				chunkTime.dwSize = dwChunkSize;
				chunkTime.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x2E001008:
				chunkSkin.dwId = dwChunkId;
				chunkSkin.dwSize = dwChunkSize;
				chunkSkin.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x0301C000:
			case 0x2E002000:
				chunkType.dwId = dwChunkId;
				chunkType.dwSize = dwChunkSize;
				chunkType.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x0301C001:
			case 0x2E002001:
				chunkVersion.dwId = dwChunkId;
				chunkVersion.dwSize = dwChunkSize;
				chunkVersion.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03031000:
			case 0x090F4000:
				chunkGameSkin.dwId = dwChunkId;
				chunkGameSkin.dwSize = dwChunkSize;
				chunkGameSkin.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03038000:
				chunkMood.dwId = dwChunkId;
				chunkMood.dwSize = dwChunkSize;
				chunkMood.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03038001:
				chunkUnknown.dwId = dwChunkId;
				chunkUnknown.dwSize = dwChunkSize;
				chunkUnknown.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// Folder Dep chunk
	if (chunkFolder.dwSize > 0)
		bSuccess &= FolderDepChunk(hwndCtl, hFile, &chunkFolder);

	// Desc chunk
	if (chunkDesc.dwSize > 0)
		bSuccess &= CollectorDescChunk(hwndCtl, hFile, &chunkDesc);

	// Icon chunk
	if (chunkIcon.dwSize > 0)
		bSuccess &= CollectorIconChunk(hwndCtl, hFile, &chunkIcon);

	// Time chunk
	if (chunkTime.dwSize > 0)
		bSuccess &= CollectorTimeChunk(hwndCtl, hFile, &chunkTime);

	// Skin chunk
	if (chunkSkin.dwSize > 0)
		bSuccess &= CollectorSkinChunk(hwndCtl, hFile, &chunkSkin);

	// Type chunk
	if (chunkType.dwSize > 0)
		bSuccess &= ObjectInfoTypeChunk(hwndCtl, hFile, &chunkType);

	// Version chunk
	if (chunkVersion.dwSize > 0)
		bSuccess &= ObjectInfoVersionChunk(hwndCtl, hFile, &chunkVersion);

	// Game Skin chunk
	if (chunkGameSkin.dwSize > 0)
		bSuccess &= GameSkinChunk(hwndCtl, hFile, &chunkGameSkin);

	// Mood chunk
	if (chunkMood.dwSize > 0)
		bSuccess &= DecorationMoodChunk(hwndCtl, hFile, &chunkMood);

	// Unknown chunk
	if (chunkUnknown.dwSize > 0)
		bSuccess &= DecorationUnknownChunk(hwndCtl, hFile, &chunkUnknown);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpSkin(HWND hwndCtl, HANDLE hFile)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkGameSkin = {0};
	TCHAR szOutput[OUTPUT_LEN];

	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF)
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x03031000:
			case 0x090F4000:
				chunkGameSkin.dwId = dwChunkId;
				chunkGameSkin.dwSize = dwChunkSize;
				chunkGameSkin.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// Skin chunk
	if (chunkGameSkin.dwSize > 0)
		bSuccess &= GameSkinChunk(hwndCtl, hFile, &chunkGameSkin);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpProfile(HWND hwndCtl, HANDLE hFile)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkProfile = {0};
	TCHAR szOutput[OUTPUT_LEN];

	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF)
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x0308C000:
				chunkProfile.dwId = dwChunkId;
				chunkProfile.dwSize = dwChunkSize;
				chunkProfile.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// Profile chunk
	if (chunkProfile.dwSize > 0)
		bSuccess &= ProfileChunk(hwndCtl, hFile, &chunkProfile);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpCollection(HWND hwndCtl, HANDLE hFile)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkFolder = {0};
	CHUNK chunkOldDesc = {0};
	CHUNK chunkDesc = {0};
	CHUNK chunkFolders = {0};
	CHUNK chunkMenuIcons = {0};
	TCHAR szOutput[OUTPUT_LEN];

	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF)
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x01001000:
				chunkFolder.dwId = dwChunkId;
				chunkFolder.dwSize = dwChunkSize;
				chunkFolder.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x24004000:
			case 0x03033000:
				chunkOldDesc.dwId = dwChunkId;
				chunkOldDesc.dwSize = dwChunkSize;
				chunkOldDesc.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x24004001:
			case 0x03033001:
				chunkDesc.dwId = dwChunkId;
				chunkDesc.dwSize = dwChunkSize;
				chunkDesc.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03033002:
				chunkFolders.dwId = dwChunkId;
				chunkFolders.dwSize = dwChunkSize;
				chunkFolders.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			case 0x03033003:
				chunkMenuIcons.dwId = dwChunkId;
				chunkMenuIcons.dwSize = dwChunkSize;
				chunkMenuIcons.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// Folder Dep chunk
	if (chunkFolder.dwSize > 0)
		bSuccess &= FolderDepChunk(hwndCtl, hFile, &chunkFolder);

	// Old Desc chunk
	if (chunkOldDesc.dwSize > 0)
		bSuccess &= CollectionOldDescChunk(hwndCtl, hFile, &chunkOldDesc);

	// Desc chunk
	if (chunkDesc.dwSize > 0)
		bSuccess &= CollectionDescChunk(hwndCtl, hFile, &chunkDesc);

	// Collector Folders chunk
	if (chunkFolders.dwSize > 0)
		bSuccess &= CollectionFoldersChunk(hwndCtl, hFile, &chunkFolders);

	// Menu Icons Folders chunk
	if (chunkMenuIcons.dwSize > 0)
		bSuccess &= CollectionMenuIconsChunk(hwndCtl, hFile, &chunkMenuIcons);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpPlug(HWND hwndCtl, HANDLE hFile)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkVersion = {0};
	TCHAR szOutput[OUTPUT_LEN];

	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF)
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x090B0000:
			case 0x090BB000:
				chunkVersion.dwId = dwChunkId;
				chunkVersion.dwSize = dwChunkSize;
				chunkVersion.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// Version chunk
	if (chunkVersion.dwSize > 0)
		bSuccess &= PlugVersionChunk(hwndCtl, hFile, &chunkVersion);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpHms(HWND hwndCtl, HANDLE hFile)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkVersion = {0};
	TCHAR szOutput[OUTPUT_LEN];

	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF)
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x06021000:
				chunkVersion.dwId = dwChunkId;
				chunkVersion.dwSize = dwChunkSize;
				chunkVersion.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// Version chunk
	if (chunkVersion.dwSize > 0)
		bSuccess &= HmsVersionChunk(hwndCtl, hFile, &chunkVersion);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DumpOther(HWND hwndCtl, HANDLE hFile)
{
	DWORD dwNumHeaderChunks = 0;
	CHUNK chunkFolder = {0};
	TCHAR szOutput[OUTPUT_LEN];

	if (!FileSeekBegin(hFile, POS_NUMBER_CHUNKS))
		return FALSE;

	if (!ReadNat32(hFile, &dwNumHeaderChunks))
		return FALSE;

	if (dwNumHeaderChunks == 0)
	{
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CHUNKS : IDP_ENG_ERR_CHUNKS);
		return TRUE;
	}
	else if (dwNumHeaderChunks > 0xFF)
		return FALSE;

	OutputText(hwndCtl, g_szSep1);

	DWORD dwChunkId, dwChunkSize;
	DWORD dwChunkOffset = GET_CHUNK_OFFSET(dwNumHeaderChunks);

	for (DWORD dwCouter = 1; dwCouter <= dwNumHeaderChunks; dwCouter++)
	{
		if (!ReadNat32(hFile, &dwChunkId))
			return FALSE;
		if (!ReadNat32(hFile, &dwChunkSize))
			return FALSE;

		dwChunkSize &= 0x7FFFFFFF;

		switch (dwChunkId)
		{
			case 0x01001000:
				chunkFolder.dwId = dwChunkId;
				chunkFolder.dwSize = dwChunkSize;
				chunkFolder.dwOffset = dwChunkOffset;
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
				break;

			default:
				dwChunkOffset += dwChunkSize;
				OutputTextFmt(hwndCtl, szOutput, g_szChunk, dwCouter, dwChunkId, dwChunkSize);
		}
	}

	BOOL bSuccess = TRUE;

	// Folder Dep chunk
	if (chunkFolder.dwSize > 0)
		bSuccess &= FolderDepChunk(hwndCtl, hFile, &chunkFolder);

	if (dwNumHeaderChunks > 1 || chunkFolder.dwSize == 0)
		OutputTextErr(hwndCtl, g_bGerUI ? IDP_GER_ERR_CLASS : IDP_ENG_ERR_CLASS);

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////

#define EFid_Resource 4

DWORD ReadRefTable(HWND hwndCtl, HANDLE hFile, WORD wVersion, PBYTE achStorageSettings)
{
	TCHAR szOutput[OUTPUT_LEN];
	BOOL bIsText = IS_GBX_TEXT(achStorageSettings);

	// Is the RefTable block compressed?
	if (IS_REF_COMPRESSED(achStorageSettings))
		return (DWORD)-1;

	// Number of external references
	DWORD dwExtEntries = 0;
	if (!ReadNat32(hFile, &dwExtEntries, bIsText))
		return (DWORD)-1;

	if (dwExtEntries > 0) OutputText(hwndCtl, g_szSep1);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Num Ext Refs:\t%d\r\n"), dwExtEntries);

	if (dwExtEntries == 0)
		return 0;

	// Number of levels up to the root directory
	DWORD dwAncestorLevel = 0;
	if (!ReadNat32(hFile, &dwAncestorLevel, bIsText))
		return (DWORD)-1;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Num Levels Up:\t%d\r\n"), dwAncestorLevel);

	// Number of subdirectories
	DWORD dwNumSubFolders = 0;
	if (!ReadNat32(hFile, &dwNumSubFolders, bIsText))
		return (DWORD)-1;

	// Read subdirectories
	DWORD dwIndex = 0;
	while (dwNumSubFolders--)
		if (!ReadSubFolders(hwndCtl, hFile, &dwIndex, NULL, bIsText))
			return (DWORD)-1;

	// Read external references
	DWORD dwCount = dwExtEntries;
	while (dwCount--)
	{
		OutputText(hwndCtl, g_szSep0);

		// Flags
		DWORD dwFlags = 0;
		if (!ReadNat32(hFile, &dwFlags, bIsText))
			return (DWORD)-1;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Flags:\t\t%08X\r\n"), dwFlags);

		// Read file name or resource index
		if ((dwFlags & EFid_Resource) == 0)
		{
			// File name
			SSIZE_T nRet = 0;
			CHAR szFolder[MAX_PATH];
			if ((nRet = ReadString(hFile, szFolder, _countof(szFolder), bIsText)) < 0)
				return (DWORD)-1;

			if (nRet > 0)
			{
				OutputText(hwndCtl, TEXT("File Name:\t"));
				ConvertGbxString(szFolder, nRet, szOutput, _countof(szOutput));
				OutputText(hwndCtl, szOutput);
			}
		}
		else
		{
			// Resource index
			DWORD dwResourceIndex = 0;
			if (!ReadNat32(hFile, &dwResourceIndex, bIsText))
				return (DWORD)-1;

			OutputTextFmt(hwndCtl, szOutput, TEXT("Resource Index:\t%d\r\n"), dwResourceIndex);
		}

		// Node index
		DWORD dwNodeIndex = 0;
		if (!ReadNat32(hFile, &dwNodeIndex, bIsText))
			return (DWORD)-1;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Node Index:\t%d\r\n"), dwNodeIndex);

		if (wVersion >= 5)
		{
			// Use file
			BOOL bUseFile = FALSE;
			if (!ReadBool(hFile, &bUseFile, bIsText))
				return (DWORD)-1;

			OutputTextFmt(hwndCtl, szOutput, TEXT("Use File:\t%s\r\n"),
				bUseFile ? g_szTrue : g_szFalse);
		}

		// Folder index
		if ((dwFlags & EFid_Resource) == 0)
		{
			DWORD dwFolderIndex = 0;
			if (!ReadNat32(hFile, &dwFolderIndex, bIsText))
				return (DWORD)-1;

			OutputTextFmt(hwndCtl, szOutput, TEXT("Folder Index:\t%d\r\n"), dwFolderIndex);

			if (dwFolderIndex == (DWORD)-1)
			{
				ULARGE_INTEGER ull;
				if (!ReadNat64(hFile, &ull))
					return (DWORD)-1;
			}
		}
	}

	return dwExtEntries;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Recursive; called by ReadRefTable
//
BOOL ReadSubFolders(HWND hwndCtl, HANDLE hFile, PDWORD pdwIndex, LPCSTR lpszFolder, BOOL bIsText)
{
	SSIZE_T nRet = 0;
	CHAR szRead[MAX_PATH];
	CHAR szFolder[MAX_PATH];
	TCHAR szOutput[OUTPUT_LEN];

	szFolder[0] = '\0';
	if (lpszFolder != NULL)
		lstrcpynA(szFolder, lpszFolder, _countof(szFolder));

	// Directory name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead), bIsText)) < 0)
		return FALSE;

	// Create path and output with index
	strncat(szFolder, szRead, _countof(szFolder) - strlen(szFolder) - 1);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Folder %u:\t"), ++(*pdwIndex));
	ConvertGbxString(szFolder, strlen(szFolder), szOutput, _countof(szOutput));
	OutputText(hwndCtl, szOutput);

	// Number of subdirectories
	DWORD dwNumSubFolders = 0;
	if (!ReadNat32(hFile, &dwNumSubFolders, bIsText))
		return FALSE;

	if (dwNumSubFolders > 0)
	{
		strncat(szFolder, "\\", _countof(szFolder) - strlen(szFolder) - 1);

		// Read further subdirectories
		while (dwNumSubFolders--)
			if (!ReadSubFolders(hwndCtl, hFile, pdwIndex, szFolder, bIsText))
				return FALSE;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// ReadSkin is called by GameSkinChunk and DecorationMoodChunk
//
BOOL ReadSkin(HWND hwndCtl, HANDLE hFile)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	TCHAR szOutput[OUTPUT_LEN];

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 5) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Folder
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, g_szFolder);
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion >= 1)
	{
		// Texture name
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Texture Name:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		// Scene ID
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Scene ID:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	// Number of entries
	BYTE cCount = 0;
	if (!ReadNat8(hFile, &cCount))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Number:\t\t%d\r\n"), (char)cCount);

	while (cCount--)
	{
		OutputText(hwndCtl, g_szSep0);

		// Class ID
		DWORD dwClassId = 0;
		if (!ReadMask(hFile, &dwClassId))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Class ID:\t%08X"), dwClassId);
		if (dwClassId == CLSID_FILEIMG_TM)
			OutputText(hwndCtl, TEXT(" (FileImg)"));
		else if (dwClassId == CLSID_FILESND_TM)
			OutputText(hwndCtl, TEXT(" (FileSnd)"));
		else if (dwClassId == CLSID_BITMAP_TM)
			OutputText(hwndCtl, TEXT(" (Bitmap)"));
		OutputText(hwndCtl, g_szCRLF);

		// Name
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Name:\t\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		// File
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("File:\t\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		if (cVersion >= 2)
		{
			// Need Mip Map
			BOOL bMipMap = FALSE;
			if (!ReadBool(hFile, &bMipMap))
				return FALSE;

			OutputTextFmt(hwndCtl, szOutput, TEXT("Need Mipmap:\t%s\r\n"),
				bMipMap ? g_szTrue : g_szFalse);
		}
	}

	if (cVersion >= 4)
	{
		// Dir Name Alt
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, g_szSep0);
			OutputText(hwndCtl, TEXT("Alt Folder:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	if (cVersion >= 5)
	{
		// Use Default Skin
		BOOL bUseDefSkin = FALSE;
		if (!ReadBool(hFile, &bUseDefSkin))
			return FALSE;

		OutputText(hwndCtl, g_szSep0);
		OutputTextFmt(hwndCtl, szOutput, TEXT("Use Def Skin:\t%s\r\n"),
			bUseDefSkin ? g_szTrue : g_szFalse);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ChallengeTmDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckTmDesc)
{
	BYTE  cVersion = 0;
	DWORD dwGold = UNASSIGNED;
	DWORD dwSilver = UNASSIGNED;
	DWORD dwBronze = UNASSIGNED;
	DWORD dwAuthortime = UNASSIGNED;
	DWORD dwAuthorscore = UNASSIGNED;
	DWORD dwTrackType = UNASSIGNED;
	DWORD dwEditorMode = UNASSIGNED;
	DWORD dwCopperPrice = 0;
	DWORD dwCheckpoints = 0;
	DWORD dwNbLaps = 0;
	BOOL  bIsLapRace = FALSE;
	BOOL  bFormatTime = TRUE;

	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckTmDesc->dwId);

	// Jump to the TmDesc chunk
	if (!FileSeekBegin(hFile, pckTmDesc->dwOffset))
		return FALSE;

	// Chunk version
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 13) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Read data...
	if (cVersion < 3)
	{	// Was the data from chunk 24003003 initially stored here?
		SSIZE_T nRet = 0;
		CHAR szRead[ID_LEN];
		IDENTIFIER id;
		ResetIdentifier(&id);
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0) return FALSE;
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0) return FALSE;
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0) return FALSE;
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0) return FALSE;
	}

	// Skip unused Bool variable
	if (!FileSeekCurrent(hFile, 4))
		return FALSE;

	if (cVersion >= 1)
	{
		// Bronze
		if (!ReadNat32(hFile, &dwBronze))
			return FALSE;

		// Silver
		if (!ReadNat32(hFile, &dwSilver))
			return FALSE;

		// Gold
		if (!ReadNat32(hFile, &dwGold))
			return FALSE;

		// Author time
		if (!ReadNat32(hFile, &dwAuthortime))
			return FALSE;
	}

	if (cVersion == 2)
	{
		// Skip unused Nat8 variable
		if (!FileSeekCurrent(hFile, 1))
			return FALSE;
	}

	if (cVersion >= 4)
	{
		// CopperPrice
		if (!ReadNat32(hFile, &dwCopperPrice))
			return FALSE;
	}

	if (cVersion >= 5)
	{
		// Multilap
		if (!ReadBool(hFile, &bIsLapRace))
			return FALSE;
	}

	if (cVersion == 6)
	{
		// Skip unused Bool variable
		if (!FileSeekCurrent(hFile, 4))
			return FALSE;
	}

	if (cVersion >= 7)
	{
		// Track Type
		if (!ReadNat32(hFile, &dwTrackType))
			return FALSE;

		// Don't convert the points to times for Platform and Stunts
		if (dwTrackType == 1 || dwTrackType == 5)
			bFormatTime = FALSE;
	}

	if (cVersion >= 9)
	{
		// Skip unused Nat32 variable
		if (!FileSeekCurrent(hFile, 4))
			return FALSE;
	}

	if (cVersion >= 10)
	{
		// Author Score
		if (!ReadNat32(hFile, &dwAuthorscore))
			return FALSE;
	}

	if (cVersion >= 11)
	{
		// Editor Mode
		if (!ReadNat32(hFile, &dwEditorMode))
			return FALSE;
	}

	if (cVersion >= 12)
	{
		// Skip unused Bool variable
		if (!FileSeekCurrent(hFile, 4))
			return FALSE;
	}

	if (cVersion >= 13)
	{
		// Checkpoints
		if (!ReadNat32(hFile, &dwCheckpoints))
			return FALSE;

		// Number of laps
		if (!ReadNat32(hFile, &dwNbLaps))
			return FALSE;
	}

	// Output data...
	if (cVersion >= 1)
	{
		// Bronze
		OutputText(hwndCtl, TEXT("Bronze:\t\t"));
		FormatTime(dwBronze, szOutput, _countof(szOutput), bFormatTime);
		OutputText(hwndCtl, szOutput);

		// Silver
		OutputText(hwndCtl, TEXT("Silver:\t\t"));
		FormatTime(dwSilver, szOutput, _countof(szOutput), bFormatTime);
		OutputText(hwndCtl, szOutput);

		// Gold
		OutputText(hwndCtl, TEXT("Gold:\t\t"));
		FormatTime(dwGold, szOutput, _countof(szOutput), bFormatTime);
		OutputText(hwndCtl, szOutput);

		// Author time
		OutputText(hwndCtl, TEXT("Best Time:\t"));
		FormatTime(dwAuthortime, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion >= 4)
	{
		// CopperPrice
		OutputTextFmt(hwndCtl, szOutput,
			cVersion >= 12 ? TEXT("Display Cost:\t%d\r\n") : TEXT("Copper Price:\t%d\r\n"), dwCopperPrice);
	}

	if (cVersion >= 5)
	{
		// Multilap
		OutputTextFmt(hwndCtl, szOutput, TEXT("Lap Race:\t%s\r\n"), bIsLapRace ? g_szTrue : g_szFalse);
	}

	if (cVersion >= 7)
	{
		// Track Type
		OutputText(hwndCtl, TEXT("Type:\t\t"));
		switch (dwTrackType)
		{
			case 0:
				OutputText(hwndCtl, TEXT("Race"));
				break;
			case 1:
				OutputText(hwndCtl, TEXT("Platform"));
				break;
			case 2:
				OutputText(hwndCtl, TEXT("Puzzle"));
				break;
			case 3:
				OutputText(hwndCtl, TEXT("Crazy"));
				break;
			case 4:
				OutputText(hwndCtl, TEXT("Shortcut"));
				break;
			case 5:
				OutputText(hwndCtl, TEXT("Stunts"));
				break;
			case 6:
				OutputText(hwndCtl, TEXT("Script"));
				break;
			case UNASSIGNED:
				OutputText(hwndCtl, g_szUnknown);
				break;
			default:
				OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), dwTrackType);
		}
		OutputText(hwndCtl, g_szCRLF);
	}

	if (cVersion >= 10)
	{
		// Author Score
		OutputTextFmt(hwndCtl, szOutput, TEXT("Best Score:\t%d\r\n"), dwAuthorscore);
	}

	if (cVersion >= 11)
	{
		// Editor Mode
#ifdef _DEBUG
		OutputTextFmt(hwndCtl, szOutput, TEXT("Editor Flags:\t%08X\r\n"), dwEditorMode);
#endif
		OutputTextFmt(hwndCtl, szOutput, TEXT("Simple Editor:\t%s\r\n"),
			IS_UNASSIGNED(dwEditorMode) ? g_szUnknown : ((dwEditorMode & 0x1) ? g_szTrue : g_szFalse));
		OutputTextFmt(hwndCtl, szOutput, TEXT("Party Editor:\t%s\r\n"),
			IS_UNASSIGNED(dwEditorMode) ? g_szUnknown : ((dwEditorMode & 0x4) ? g_szTrue : g_szFalse));
		OutputTextFmt(hwndCtl, szOutput, TEXT("Ghost Blocks:\t%s\r\n"),
			IS_UNASSIGNED(dwEditorMode) ? g_szUnknown : ((dwEditorMode & 0x2) ? g_szTrue : g_szFalse));
	}

	if (cVersion >= 13)
	{
		// Checkpoints
		OutputTextFmt(hwndCtl, szOutput, TEXT("Checkpoints:\t%d\r\n"), dwCheckpoints);

		// Number of laps
		OutputTextFmt(hwndCtl, szOutput, TEXT("Number Laps:\t%d\r\n"), dwNbLaps);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ChallengeCommonChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckCommon, LPSTR lpszUid, LPSTR lpszEnvi)
{
	if (lpszUid == NULL || lpszEnvi == NULL)
		return FALSE;

	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	IDENTIFIER id;
	ResetIdentifier(&id);

	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckCommon->dwId);

	// Jump to Common chunk
	if (!FileSeekBegin(hFile, pckCommon->dwOffset))
		return FALSE;

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 11) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Map UID
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		lstrcpynA(lpszUid, szRead, UID_LENGTH);
		OutputText(hwndCtl, TEXT("Map UID:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Environment
	DWORD dwId = 0;
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead), &dwId)) < 0)
		return FALSE;

	if (nRet > 0)
	{
		lstrcpynA(lpszEnvi, szRead, ENVI_LENGTH);
		OutputText(hwndCtl, TEXT("Collection:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (lpszEnvi[0] != '\0')
	{
		if (strstr(g_szMpEnvis, lpszEnvi) != NULL)
		{
			BOOL bIsTM2020 = (dwId == 26);
			Button_Enable(GetDlgItem(GetParent(hwndCtl), IDC_TMX), TRUE);
			if (strstr(g_szTm2Envis, lpszEnvi) != NULL && !bIsTM2020)
				Button_Enable(GetDlgItem(GetParent(hwndCtl), IDC_DEDIMANIA), TRUE);
		}
		else if (strstr(g_szTm1Envis, lpszEnvi) != NULL)
		{
			Button_Enable(GetDlgItem(GetParent(hwndCtl), IDC_TMX), TRUE);
			Button_Enable(GetDlgItem(GetParent(hwndCtl), IDC_DEDIMANIA), TRUE);
		}
	}

	// Author Name
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Map Author:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Track Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Map Name:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	// Game Mode
	BYTE cGameMode = 0xFF;
	if (!ReadNat8(hFile, &cGameMode))
		return FALSE;

	OutputText(hwndCtl, TEXT("Kind:\t\t"));
	switch (cGameMode)
	{
		case 0:
			OutputText(hwndCtl, TEXT("(internal)EndMarker"));
			break;
		case 1:
			OutputText(hwndCtl, TEXT("(old)Campaign"));
			break;
		case 2:
			OutputText(hwndCtl, TEXT("(old)Puzzle"));
			break;
		case 3:
			OutputText(hwndCtl, TEXT("(old)Retro"));
			break;
		case 4:
			OutputText(hwndCtl, TEXT("(old)TimeAttack"));
			break;
		case 5:
			OutputText(hwndCtl, TEXT("(old)Rounds"));
			break;
		case 6:
			OutputText(hwndCtl, TEXT("InProgress"));
			break;
		case 7:
			OutputText(hwndCtl, TEXT("Campaign"));
			break;
		case 8:
			OutputText(hwndCtl, TEXT("Multi"));
			break;
		case 9:
			OutputText(hwndCtl, TEXT("Solo"));
			break;
		case 10:
			OutputText(hwndCtl, TEXT("Site"));
			break;
		case 11:
			OutputText(hwndCtl, TEXT("SoloNadeo"));
			break;
		case 12:
			OutputText(hwndCtl, TEXT("MultiNadeo"));
			break;
		case 0xFF:
			OutputText(hwndCtl, g_szUnknown);
			break;
		default:
			OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), (char)cGameMode);
	}
	OutputText(hwndCtl, g_szCRLF);

	if (cVersion < 1)
		return TRUE;

	// Lock settings (VSK)
	BOOL bLocked = FALSE;
	if (!ReadBool(hFile, &bLocked))
		return FALSE;

	if (bLocked) // Usage in TM unknown
		OutputTextFmt(hwndCtl, szOutput, TEXT("Locked:\t\t%s\r\n"),
			bLocked ? g_szTrue : g_szFalse);

	// Password (obsolete)
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0) // Show only if set
		OutputTextFmt(hwndCtl, szOutput, TEXT("Password:\t%s\r\n"),
			nRet > 0 ? g_szYes : g_szNo);

	if (cVersion < 2)
		return TRUE;

	// Mood
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Mood:\t\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Decoration
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Decoration:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Decoration Author
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Deco Author:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 3)
		return TRUE;

	// Skip Map Coord Origin
	if (!FileSeekCurrent(hFile, 8))
		return FALSE;

	if (cVersion < 4)
		return TRUE;

	// Skip Map Coord Target
	if (!FileSeekCurrent(hFile, 8))
		return FALSE;

	if (cVersion < 5)
		return TRUE;

	// Skip unused Nat128 variable (16 bytes)
	if (!FileSeekCurrent(hFile, 16))
		return FALSE;

	if (cVersion < 6)
		return TRUE;

	// Map Type
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Map Type:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	// Map Style
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Map Style:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion <= 8)
	{
		// Skip unknown Bool variable
		if (!FileSeekCurrent(hFile, 4))
			return FALSE;
	}

	if (cVersion < 8)
		return TRUE;

	// Lightmap Cache checksum
	ULARGE_INTEGER ullLightmapCache;
	ullLightmapCache.QuadPart = 0i64;
	if (!ReadNat64(hFile, &ullLightmapCache))
		return FALSE;

	if (ullLightmapCache.HighPart != 0xFFFFFFFF || ullLightmapCache.LowPart != 0xFFFFFFFF)
		OutputTextFmt(hwndCtl, szOutput, TEXT("Lightmap Cache:\t%08X%08X\r\n"),
			ullLightmapCache.HighPart, ullLightmapCache.LowPart);

	if (cVersion < 9)
		return TRUE;

	// Lightmap Version
	BYTE cLightmap = 0;
	if (!ReadNat8(hFile, &cLightmap))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Lightmap Vers.:\t%d\r\n"), (char)cLightmap);

	if (cVersion < 11)
		return TRUE;

	// Title ID
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Title ID:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ChallengeVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckVersion)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckVersion->dwId);

	// Jump to Version chunk
	if (!FileSeekBegin(hFile, pckVersion->dwOffset))
		return FALSE;

	// Version
	DWORD dwVersion;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	// VSK requires special treatment
	OutputTextFmt(hwndCtl, szOutput, TEXT("Header Version:\t%d\r\n"),
		(dwVersion < 10000 || IS_UNASSIGNED(dwVersion)) ? dwVersion : dwVersion - 9999);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ChallengeVskDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckVskDesc)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	IDENTIFIER id;
	ResetIdentifier(&id);

	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckVskDesc->dwId);

	// Jump to the VskDesc chunk
	if (!FileSeekBegin(hFile, pckVskDesc->dwOffset))
		return FALSE;

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 14) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	if (cVersion < 1)
	{	// Was the data from chunk 24003003 initially stored here?
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0) return FALSE;
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0) return FALSE;
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0) return FALSE;
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0) return FALSE;
	}

	// Skip unknown Bool and Nat32 variables
	if (!FileSeekCurrent(hFile, 8))
		return FALSE;

	if (cVersion < 1)
	{
		// Skip unknown Nat8 variable
		if (!FileSeekCurrent(hFile, 1))
			return FALSE;
	}

	// Skip unknown Nat8 variable
	if (!FileSeekCurrent(hFile, 1))
		return FALSE;

	// Boat
	if (cVersion < 9)
	{
		BYTE cBoat = 0xFF;
		if (!ReadNat8(hFile, &cBoat))
			return FALSE;

		OutputText(hwndCtl, TEXT("Boat Name:\t"));
		switch (cBoat)
		{
			case 0:
				OutputText(hwndCtl, TEXT("Acc"));
				break;
			case 1:
				OutputText(hwndCtl, TEXT("Multi"));
				break;
			case 2:
				OutputText(hwndCtl, TEXT("Melges"));
				break;
			case 3:
				OutputText(hwndCtl, TEXT("OffShore"));
				break;
			case 0xFF:
				OutputText(hwndCtl, g_szUnknown);
				break;
			default:
				OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), (char)cBoat);
		}
		OutputText(hwndCtl, g_szCRLF);
	}
	else
	{
		// Boat
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			PSTR pszBegin = szRead;
			PSTR pszEnd = strchr(pszBegin, '\n');
			if (pszEnd != NULL)
			{
				*pszEnd = '\0';
				pszEnd++;
			}

			OutputText(hwndCtl, TEXT("Boat Name:\t"));
			ConvertGbxString(pszBegin, strlen(pszBegin), szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);

			pszBegin = pszEnd;
			if (pszBegin != NULL && *pszBegin != '\0')
			{
				pszEnd = strchr(pszBegin, '\n');
				if (pszEnd != NULL)
				{
					*pszEnd = '\0';
					pszEnd++;
				}

				OutputText(hwndCtl, TEXT("Website:\t"));
				ConvertGbxString(pszBegin, strlen(pszBegin), szOutput, _countof(szOutput));
				OutputText(hwndCtl, szOutput);

				pszBegin = pszEnd;
				if (pszBegin != NULL && *pszBegin != '\0')
				{
					pszEnd = strchr(pszBegin, '\n');
					if (pszEnd != NULL)
						*pszEnd = '\0';

					OutputText(hwndCtl, TEXT("Boat ID:\t"));
					ConvertGbxString(pszBegin, strlen(pszBegin), szOutput, _countof(szOutput));
					OutputText(hwndCtl, szOutput);
				}
			}
		}
	}

	if (cVersion >= 12)
	{
		// Author Name
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Boat Author:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	// Race Mode
	BYTE cRaceMode = 0xFF;
	if (!ReadNat8(hFile, &cRaceMode))
		return FALSE;

	OutputText(hwndCtl, TEXT("Race Mode:\t"));
	switch (cRaceMode)
	{
		case 0:
			OutputText(hwndCtl, TEXT("Fleet Race"));
			break;
		case 1:
			OutputText(hwndCtl, TEXT("Match Race"));
			break;
		case 2:
			OutputText(hwndCtl, TEXT("Team Race"));
			break;
		case 0xFF:
			OutputText(hwndCtl, g_szUnknown);
			break;
		default:
			OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), (char)cRaceMode);
	}
	OutputText(hwndCtl, g_szCRLF);

	// Skip unknown Nat8 variable
	if (!FileSeekCurrent(hFile, 1))
		return FALSE;

	// Wind Direction
	BYTE cWindDir = 0xFF;
	if (!ReadNat8(hFile, &cWindDir))
		return FALSE;

	OutputText(hwndCtl, TEXT("Wind Direction:\t"));
	switch (cWindDir)
	{
		case 0:
			OutputText(hwndCtl, TEXT("North"));
			break;
		case 1:
			OutputText(hwndCtl, TEXT("NE"));
			break;
		case 2:
			OutputText(hwndCtl, TEXT("East"));
			break;
		case 3:
			OutputText(hwndCtl, TEXT("SE"));
			break;
		case 4:
			OutputText(hwndCtl, TEXT("South"));
			break;
		case 5:
			OutputText(hwndCtl, TEXT("SW"));
			break;
		case 6:
			OutputText(hwndCtl, TEXT("West"));
			break;
		case 7:
			OutputText(hwndCtl, TEXT("NW"));
			break;
		case 0xFF:
			OutputText(hwndCtl, g_szUnknown);
			break;
		default:
			OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), (char)cWindDir);
	}
	OutputText(hwndCtl, g_szCRLF);

	// Wind Strength
	BYTE cWindStrength = 0xFF;
	if (!ReadNat8(hFile, &cWindStrength))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Wind Strength:\tForce %d\r\n"),
		cWindStrength == 0xFF ? (char)cWindStrength : cWindStrength + 3);

	// Weather
	BYTE cWeather = 0xFF;
	if (!ReadNat8(hFile, &cWeather))
		return FALSE;

	OutputText(hwndCtl, TEXT("Weather:\t"));
	switch (cWeather)
	{
		case 0:
			OutputText(hwndCtl, TEXT("Sunny"));
			break;
		case 1:
			OutputText(hwndCtl, TEXT("Cloudy"));
			break;
		case 2:
			OutputText(hwndCtl, TEXT("Rainy"));
			break;
		case 3:
			OutputText(hwndCtl, TEXT("Stormy"));
			break;
		case 0xFF:
			OutputText(hwndCtl, g_szUnknown);
			break;
		default:
			OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), (char)cWeather);
	}
	OutputText(hwndCtl, g_szCRLF);

	// Skip unknown Nat8 variable
	if (!FileSeekCurrent(hFile, 1))
		return FALSE;

	// Start Delay
	BYTE cStartDelay = 0xFF;
	if (!ReadNat8(hFile, &cStartDelay))
		return FALSE;

	OutputText(hwndCtl, TEXT("Start Delay:\t"));
	switch (cStartDelay)
	{
		case 0:
			OutputText(hwndCtl, TEXT("Immediate"));
			break;
		case 1:
			OutputText(hwndCtl, TEXT("1 Min"));
			break;
		case 2:
			OutputText(hwndCtl, TEXT("3 Min"));
			break;
		case 3:
			OutputText(hwndCtl, TEXT("5 Min"));
			break;
		case 4:
			OutputText(hwndCtl, TEXT("8 Min"));
			break;
		case 0xFF:
			OutputText(hwndCtl, g_szUnknown);
			break;
		default:
			OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), (char)cStartDelay);
	}
	OutputText(hwndCtl, g_szCRLF);

	// Start Time
	DWORD dwDayTime = UNASSIGNED;
	if (!ReadNat32(hFile, &dwDayTime))
		return FALSE;

	OutputText(hwndCtl, TEXT("Start Time:\t"));
	FormatTime(dwDayTime, szOutput, _countof(szOutput));
	OutputText(hwndCtl, szOutput);

	if (cVersion < 2)
		return TRUE;

	// Time Limit
	DWORD dwTimeLimit = UNASSIGNED;
	if (!ReadNat32(hFile, &dwTimeLimit))
		return FALSE;

	OutputText(hwndCtl, TEXT("Time Limit:\t"));
	FormatTime(dwTimeLimit, szOutput, _countof(szOutput));
	OutputText(hwndCtl, szOutput);

	// No Penalty
	BOOL bNoPenalty = FALSE;
	if (!ReadBool(hFile, &bNoPenalty))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("No Penalty:\t%s\r\n"),
		bNoPenalty ? g_szYes : g_szNo);

	// Infl. Penalty
	BOOL bInflPenalty = FALSE;
	if (!ReadBool(hFile, &bInflPenalty))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Infl. Penalty:\t%s\r\n"),
		bInflPenalty ? g_szYes : g_szNo);

	// Finish First
	BOOL bFinishFirst = FALSE;
	if (!ReadBool(hFile, &bFinishFirst))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Finish First:\t%s\r\n"),
		bFinishFirst ? g_szYes : g_szNo);

	if (cVersion < 3)
		return TRUE;

	// Number of AIs
	BYTE cNbAIs = 0;
	if (!ReadNat8(hFile, &cNbAIs))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Number AIs:\t%d\r\n"), (char)cNbAIs);

	if (cVersion < 4)
		return TRUE;

	// Course Length
	FLOAT fLength = 0.0f;
	if (!ReadReal(hFile, &fLength))
		return FALSE;

	_sntprintf(szOutput, _countof(szOutput), TEXT("Course Length:\t%f nmi\r\n"), (double)fLength);
	OutputText(hwndCtl, szOutput);

	if (cVersion == 4)
	{
		// Skip unknown Nat8 variable
		if (!FileSeekCurrent(hFile, 1))
			return FALSE;
	}

	// Wind Shift Duration
	DWORD dwWindShiftDur = UNASSIGNED;
	if (!ReadNat32(hFile, &dwWindShiftDur))
		return FALSE;

	OutputText(hwndCtl, TEXT("Wind Shift Dur:\t"));
	FormatTime(dwWindShiftDur, szOutput, _countof(szOutput));
	OutputText(hwndCtl, szOutput);

	if (cVersion < 5)
		return TRUE;

	// Wind Shift Angle
	int nWindShiftAng = 0;
	if (!ReadInteger(hFile, &nWindShiftAng))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Wind Shift Ang:\t%d°\r\n"), nWindShiftAng);

	// Skip unknown Nat8 variable
	if (!FileSeekCurrent(hFile, 1))
		return FALSE;

	if (cVersion == 6 || cVersion == 7)
	{
		// Skip unknown Bool variable
		if (!FileSeekCurrent(hFile, 4))
			return FALSE;

		// Unknown
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Unknown:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	if (cVersion < 7)
		return TRUE;

	// Exact Wind
	BOOL bExactWind = FALSE;
	if (!ReadBool(hFile, &bExactWind))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Exact Wind:\t%s\r\n"),
		bExactWind ? g_szFalse : g_szTrue);

	if (cVersion < 10)
		return TRUE;

	// Spawn Points
	DWORD dwSpawnPoints = 0;
	if (!ReadNat32(hFile, &dwSpawnPoints))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Spawn Points:\t%u\r\n"), dwSpawnPoints);

	if (cVersion < 11)
		return TRUE;

	// AI Level
	BYTE cAILevel = 0xFF;
	if (!ReadNat8(hFile, &cAILevel))
		return FALSE;

	OutputText(hwndCtl, TEXT("AI Level:\t"));
	switch (cAILevel)
	{
		case 0:
			OutputText(hwndCtl, TEXT("Easy"));
			break;
		case 1:
			OutputText(hwndCtl, TEXT("Intermediate"));
			break;
		case 2:
			OutputText(hwndCtl, TEXT("Expert"));
			break;
		case 3:
			OutputText(hwndCtl, TEXT("Pro"));
			break;
		case 0xFF:
			OutputText(hwndCtl, g_szUnknown);
			break;
		default:
			OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), (char)cAILevel);
	}
	OutputText(hwndCtl, g_szCRLF);

	if (cVersion < 13)
		return TRUE;

	// Small Shifts +/-3 degree
	BOOL bSmallShifts = FALSE;
	if (!ReadBool(hFile, &bSmallShifts))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Small Shifts:\t%s\r\n"),
		bSmallShifts ? g_szTrue : g_szFalse);

	if (cVersion < 14)
		return TRUE;

	// No ISAF rules
	BOOL bNoRules = FALSE;
	if (!ReadBool(hFile, &bNoRules))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("No Rules:\t%s\r\n"),
		bNoRules ? g_szYes : g_szNo);

	// Start with sail up
	BOOL bStartSailUp = FALSE;
	if (!ReadBool(hFile, &bStartSailUp))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Start Sail Up:\t%s\r\n"),
		bStartSailUp ? g_szYes : g_szNo);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ChallengeThumbnailChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckThumbnail)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckThumbnail->dwId);

	// Jump to Thumbnail chunk
	if (!FileSeekBegin(hFile, pckThumbnail->dwOffset))
		return FALSE;

	// Chunk version
	DWORD dwVersion = 0;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, dwVersion);
	if (dwVersion > 1) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	if (dwVersion != 0)
	{
		// Read length of Thumbnail chunk
		DWORD dwThumbnailSize = 0;
		if (!ReadNat32(hFile, &dwThumbnailSize) || dwThumbnailSize > 0xA0000)
			return FALSE;

		// <Thumbnail.jpg>
		SIZE_T cbLen = sizeof "<Thumbnail.jpg>" - 1;
		LPVOID lpData = GlobalAllocPtr(GHND, cbLen + 1); // 1 additional character for terminating zero
		if (lpData == NULL)
			return FALSE;

		if (!ReadData(hFile, lpData, cbLen))
		{
			GlobalFreePtr(lpData);
			return FALSE;
		}

		// Output <Thumbnail.jpg>
		ConvertGbxString(lpData, cbLen, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);

		GlobalFreePtr(lpData);

		if (dwThumbnailSize > 0)
		{
			if (FormatByteSize(dwThumbnailSize, szOutput, _countof(szOutput)))
			{
				OutputText(hwndCtl, TEXT("Image Size:\t"));
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, g_szCRLF);
			}

			lpData = GlobalAllocPtr(GHND, dwThumbnailSize);
			if (lpData == NULL)
				return FALSE;

			if (!ReadData(hFile, lpData, dwThumbnailSize))
			{
				GlobalFreePtr(lpData);
				return FALSE;
			}

			// Decode the thumbnail image
			HANDLE hDib = NULL;
			__try { hDib = JpegToDib(lpData, dwThumbnailSize); }
			__except (EXCEPTION_EXECUTE_HANDLER) { hDib = NULL; }
			if (hDib != NULL)
			{
				if (g_hBitmapThumb != NULL)
					FreeBitmap(g_hBitmapThumb);
				g_hBitmapThumb = NULL;

				if (g_hDibThumb != NULL)
					FreeDib(g_hDibThumb);
				g_hDibThumb = hDib;

				LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(g_hDibThumb);
				if (lpbi != NULL)
				{
					LONG lWidth  = ((LPBITMAPINFOHEADER)lpbi)->biWidth;
					LONG lHeight = ((LPBITMAPINFOHEADER)lpbi)->biHeight;
					if (lHeight < 0) lHeight = -lHeight;
					GlobalUnlock(g_hDibThumb);

					OutputTextFmt(hwndCtl, szOutput, TEXT("Resolution:\t%d x %d pixels\r\n"), lWidth, lHeight);
				}

				// View the thumbnail immediately
				HWND hwndThumb = GetDlgItem(GetParent(hwndCtl), IDC_THUMB);
				if (hwndThumb != NULL)
				{
					if (InvalidateRect(hwndThumb, NULL, FALSE))
						UpdateWindow(hwndThumb);
				}
			}

			GlobalFreePtr(lpData);
		}

		// </Thumbnail.jpg>
		cbLen = sizeof "</Thumbnail.jpg>" - 1;
		lpData = GlobalAllocPtr(GHND, cbLen + 1);
		if (lpData == NULL)
			return FALSE;

		if (!ReadData(hFile, lpData, cbLen))
		{
			GlobalFreePtr(lpData);
			return FALSE;
		}

		// Output </Thumbnail.jpg>
		ConvertGbxString(lpData, cbLen, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);

		GlobalFreePtr(lpData);

		// <Comments>
		cbLen = sizeof "<Comments>" - 1;
		lpData = GlobalAllocPtr(GHND, cbLen + 1);
		if (lpData == NULL)
			return FALSE;

		if (!ReadData(hFile, lpData, cbLen))
		{
			GlobalFreePtr(lpData);
			return FALSE;
		}

		// Output <Comments>
		ConvertGbxString(lpData, cbLen, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);

		GlobalFreePtr(lpData);

		// Comments
		DWORD dwCommentsSize = 0;
		if (!ReadNat32(hFile, &dwCommentsSize) || dwCommentsSize >= 0xFFFF)
			return FALSE;

		if (dwCommentsSize > 0)
		{
			lpData = GlobalAllocPtr(GHND, (SIZE_T)dwCommentsSize + 1);
			if (lpData == NULL)
				return FALSE;

			if (!ReadData(hFile, lpData, dwCommentsSize))
			{
				GlobalFreePtr(lpData);
				return FALSE;
			}

			// Output comment
			ConvertGbxString(lpData, dwCommentsSize, szOutput, _countof(szOutput), TRUE);
			OutputText(hwndCtl, szOutput);

			GlobalFreePtr(lpData);
		}

		// </Comments>
		cbLen = sizeof "</Comments>" - 1;
		lpData = GlobalAllocPtr(GHND, cbLen + 1);
		if (lpData == NULL)
			return FALSE;

		if (!ReadData(hFile, lpData, cbLen))
		{
			GlobalFreePtr(lpData);
			return FALSE;
		}

		// Output </Comments>
		ConvertGbxString(lpData, cbLen, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);

		GlobalFreePtr(lpData);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ReplayVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckVersion, LPSTR lpszUid, LPSTR lpszEnvi)
{
	if (lpszUid == NULL || lpszEnvi == NULL)
		return FALSE;

	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	IDENTIFIER id;
	ResetIdentifier(&id);

	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckVersion->dwId);

	// Jump to Version chunk
	if (!FileSeekBegin(hFile, pckVersion->dwOffset))
		return FALSE;

	// Chunk version
	DWORD dwVersion = 0;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	// Virtual Skipper requires special handling
	OutputTextFmt(hwndCtl, szOutput, TEXT("Header Version:\t%d\r\n"),
		(dwVersion < 10000 || IS_UNASSIGNED(dwVersion)) ? dwVersion : dwVersion - 9999);

	BOOL bIsVSK = dwVersion >= 9999 ? TRUE : FALSE;

	// Check whether further data is available. This is necessary
	// for VSK, because all replays have the version number 10000.
	if (pckVersion->dwSize <= 4)
		return TRUE;

	if ((!bIsVSK && dwVersion >= 3) || (bIsVSK && dwVersion >= 10000))
	{
		// Map UID
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			lstrcpynA(lpszUid, szRead, UID_LENGTH);
			OutputText(hwndCtl, TEXT("Map UID:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		// Environment
		DWORD dwId = 0;
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead), &dwId)) < 0)
			return FALSE;

		if (nRet > 0)
		{
			lstrcpynA(lpszEnvi, szRead, ENVI_LENGTH);
			OutputText(hwndCtl, TEXT("Collection:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		if (lpszEnvi[0] != '\0')
		{
			if (strstr(g_szMpEnvis, lpszEnvi) != NULL)
			{
				BOOL bIsTM2020 = (dwId == 26);
				Button_Enable(GetDlgItem(GetParent(hwndCtl), IDC_TMX), TRUE);
				if (strstr(g_szTm2Envis, lpszEnvi) != NULL && !bIsTM2020)
					Button_Enable(GetDlgItem(GetParent(hwndCtl), IDC_DEDIMANIA), TRUE);
			}
			else if (strstr(g_szTm1Envis, lpszEnvi) != NULL)
			{
				Button_Enable(GetDlgItem(GetParent(hwndCtl), IDC_TMX), TRUE);
				Button_Enable(GetDlgItem(GetParent(hwndCtl), IDC_DEDIMANIA), TRUE);
			}
		}

		// Author Name
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Map Author:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		// Best Time
		DWORD dwBest = UNASSIGNED;
		if (!ReadNat32(hFile, &dwBest))
			return FALSE;

		if (!IS_UNASSIGNED(dwBest))
		{
			OutputText(hwndCtl, TEXT("Best Time:\t"));
			FormatTime(dwBest, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}

		// Nick Name
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Player Name:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
			OutputText(hwndCtl, szOutput);
		}

		if ((!bIsVSK && dwVersion >= 6) || (bIsVSK && dwVersion >= 10000))
		{
			// Login
			if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
				return FALSE;

			// Check length, because the login is missing in VSK Replays for version 0.1.5.x.
			// This hack works because the next DWORD would be the size of the XML block.
			if (nRet > 0 && nRet <= 128)
			{
				OutputText(hwndCtl, TEXT("Player Login:\t"));
				ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
				OutputText(hwndCtl, szOutput);
			}

			if (!bIsVSK && dwVersion >= 8)
			{
				// Skip unused Nat8 variable
				if (!FileSeekCurrent(hFile, 1))
					return FALSE;

				// Title ID
				if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
					return FALSE;

				if (nRet > 0)
				{
					OutputText(hwndCtl, TEXT("Title ID:\t"));
					ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
					OutputText(hwndCtl, szOutput);
				}
			}
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ChallengeReplayCommunityChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckCommunity)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckCommunity->dwId);

	// Jump to Community chunk
	if (!FileSeekBegin(hFile, pckCommunity->dwOffset))
		return FALSE;

	// Community chunk size
	DWORD dwXmlSize = 0;
	if (!ReadNat32(hFile, &dwXmlSize))
		return FALSE;

	dwXmlSize &= 0x7FFFFFFF;
	if (dwXmlSize == 0)
		return TRUE;
	else if (dwXmlSize > 0xFFFF) // sanity check
		return FALSE;

	// Output Community chunk
	LPVOID pXmlData = GlobalAllocPtr(GHND, (SIZE_T)dwXmlSize + 1); // 1 additional character for terminating null
	if (pXmlData == NULL)
		return FALSE;

	if (!ReadData(hFile, pXmlData, dwXmlSize))
	{
		GlobalFreePtr(pXmlData);
		return FALSE;
	}

	// Allocate memory for Unicode
	LPVOID pXmlString = GlobalAllocPtr(GHND, 2 * ((SIZE_T)dwXmlSize + 3));
	if (pXmlString != NULL)
	{
		__try
		{
			// Convert to Unicode
			ConvertGbxString(pXmlData, dwXmlSize, (LPTSTR)pXmlString, (SIZE_T)dwXmlSize + 3);

			// Insert line breaks
			LPCTSTR lpszTemp = AllocReplaceString((LPCTSTR)pXmlString, TEXT("><"), TEXT(">\r\n<"));
			if (lpszTemp == NULL)
				OutputText(hwndCtl, (LPCTSTR)pXmlString);
			else
			{
				OutputText(hwndCtl, lpszTemp);
				GlobalFreePtr((LPVOID)lpszTemp);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { ; }

		GlobalFreePtr(pXmlString);
	}

	GlobalFreePtr(pXmlData);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ChallengeReplayAuthorChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckAuthor)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckAuthor->dwId);

	// Jump to Author chunk
	if (!FileSeekBegin(hFile, pckAuthor->dwOffset))
		return FALSE;

	// Version
	DWORD dwVersion;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, dwVersion);
	if (dwVersion > 1) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// AuthorInfo version
	DWORD dwAuthorVer;
	if (!ReadNat32(hFile, &dwAuthorVer))
		return FALSE;

#ifndef _DEBUG
	if (dwAuthorVer > 0)
#endif
	{ // Display only if changed
		OutputTextFmt(hwndCtl, szOutput, TEXT("Author Version:\t%d"), dwAuthorVer);
		if (dwAuthorVer > 0) OutputText(hwndCtl, g_szAsterisk);
		OutputText(hwndCtl, g_szCRLF);
	}

	// Login
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Author Login:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Nick Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Author Name:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput), TRUE);
		OutputText(hwndCtl, szOutput);
	}

	// Zone
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Author Zone:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Extra Info
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Extra Info:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CollectorDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckDesc)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	IDENTIFIER id;
	ResetIdentifier(&id);

	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckDesc->dwId);

	// Jump to Desc chunk
	if (!FileSeekBegin(hFile, pckDesc->dwOffset))
		return FALSE;

	// Name
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Name:\t\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Collection
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Collection:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Author Name
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Author:\t\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// The following identifier is used as version number
	DWORD dwVersion = 0;
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead), &dwVersion)) < 0)
		return FALSE;

	if (!IS_NUMBER(dwVersion))
		dwVersion = 0;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Version:\t%d"), dwVersion);
	if (dwVersion > 8) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Page Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Page Name:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (dwVersion == 5)
	{
		// Unknown
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Unknown:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	if (dwVersion >= 4)
	{
		// Unknown
		if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Unknown:\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	if (dwVersion >= 3)
	{
		// Flags
		DWORD dwFlags = 0;
		if (!ReadNat32(hFile, &dwFlags))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Flags:\t\t%08X"), dwFlags & 0x3FF);

		if (dwVersion >= 6)
		{
			typedef struct SCollectorDescFlags
			{
				UINT __unused2__ : 1;
				UINT IsInternal  : 1;
				UINT IsAdvanced  : 1;
				UINT IconDesc    : 5;
				UINT __unused__  : 24;
			} COLLECTORDESCFLAGS, *PCOLLECTORDESCFLAGS;

			PCOLLECTORDESCFLAGS pFlags = (PCOLLECTORDESCFLAGS)&dwFlags;

			szOutput[0] = g_chNil;

			if (pFlags->IsInternal)
				AppendFlagName(szOutput, _countof(szOutput), TEXT("IsInternal"));

			if (pFlags->IsAdvanced)
				AppendFlagName(szOutput, _countof(szOutput), TEXT("IsAdvanced"));

			switch (pFlags->IconDesc)
			{
				//case 0:
				//	AppendFlagName(szOutput, _countof(szOutput), TEXT("IconDesc_Unknown"));
				//	break;
				case 1:
					AppendFlagName(szOutput, _countof(szOutput), TEXT("IconDesc_NoIcon"));
					break;
				case 2:
					AppendFlagName(szOutput, _countof(szOutput), TEXT("IconDesc_64x64"));
					break;
				case 3:
					AppendFlagName(szOutput, _countof(szOutput), TEXT("IconDesc_128x128"));
					break;
			}

			if (szOutput[0] != g_chNil)
			{
				OutputText(hwndCtl, TEXT(" ("));
				OutputText(hwndCtl, szOutput);
				OutputText(hwndCtl, TEXT(")"));
			}
		}

		OutputText(hwndCtl, g_szCRLF);

		// Catalog Position
		WORD wIndex = 0;
		if (!ReadNat16(hFile, &wIndex))
			return FALSE;

		OutputTextFmt(hwndCtl, szOutput, TEXT("Position:\t%d\r\n"), (short)wIndex);
	}

	if (dwVersion >= 7)
	{
		// Name
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, TEXT("Name:\t\t"));
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	if (dwVersion >= 8)
	{
		// Prod State
		BYTE cProdState = 0;
		if (!ReadNat8(hFile, &cProdState))
			return FALSE;

		OutputText(hwndCtl, TEXT("Prod State:\t"));
		switch (cProdState)
		{
			case 0:
				OutputText(hwndCtl, TEXT("Aborted"));
				break;
			case 1:
				OutputText(hwndCtl, TEXT("GameBox"));
				break;
			case 2:
				OutputText(hwndCtl, TEXT("DevBuild"));
				break;
			case 3:
				OutputText(hwndCtl, TEXT("Release"));
				break;
			case 0xFF:
				OutputText(hwndCtl, g_szUnknown);
				break;
			default:
				OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), (char)cProdState);
		}
		OutputText(hwndCtl, g_szCRLF);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CollectorIconChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckIcon)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckIcon->dwId);

	// Jump to Icon chunk
	if (!FileSeekBegin(hFile, pckIcon->dwOffset))
		return FALSE;

	// Determine icon size
	WORD wVersion = 0;
	WORD wWidth  = 0;
	WORD wHeight = 0;

	if (!ReadNat16(hFile, &wWidth))
		return FALSE;
	if (!ReadNat16(hFile, &wHeight))
		return FALSE;

	if ((SHORT)wWidth < 0 || (SHORT)wHeight < 0)
	{
		wVersion = 1;
		wWidth &= 0x07FF;
		wHeight &= 0x07FF;
	}

	OutputTextFmt(hwndCtl, szOutput, TEXT("Icon Width:\t%u pixels\r\n"), wWidth);
	OutputTextFmt(hwndCtl, szOutput, TEXT("Icon Height:\t%u pixels\r\n"), wHeight);

	if (wVersion < 1)
	{ // Read RGBA image
		// Determining and checking the amount of image data
		DWORD dwSizeImage = wWidth * wHeight * 4;
		if (dwSizeImage == 0 || dwSizeImage != (pckIcon->dwSize - 4))
			return TRUE;	// no, compressed or corrupted data

		// Allocate memory
		LPVOID lpData = GlobalAllocPtr(GHND, dwSizeImage);
		if (lpData == NULL)
			return FALSE;

		// Read image data
		if (!ReadData(hFile, lpData, pckIcon->dwSize - 4))
		{
			GlobalFreePtr(lpData);
			return FALSE;
		}

		// Create a 32-bit DIB
		HANDLE hDib = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + dwSizeImage);
		if (hDib != NULL)
		{
			if (g_hBitmapThumb != NULL)
			{
				FreeBitmap(g_hBitmapThumb);
				g_hBitmapThumb = NULL;
			}
			if (g_hDibThumb != NULL)
			{
				FreeDib(g_hDibThumb);
				g_hDibThumb = NULL;
			}

			LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
			if (lpbi != NULL)
			{
				lpbi->biSize = sizeof(BITMAPINFOHEADER);
				lpbi->biWidth = wWidth;
				lpbi->biHeight = wHeight;
				lpbi->biPlanes = 1;
				lpbi->biBitCount = 32;
				lpbi->biCompression = BI_RGB;
				lpbi->biSizeImage = dwSizeImage;
				lpbi->biXPelsPerMeter = 0;
				lpbi->biYPelsPerMeter = 0;
				lpbi->biClrUsed = 0;
				lpbi->biClrImportant = 0;

				register LPBYTE lpSrc = (LPBYTE)lpData;
				register LPBYTE lpDest = ((LPBYTE)lpbi) + sizeof(BITMAPINFOHEADER);

				__try
				{
					while (dwSizeImage--)
						*lpDest++ = *lpSrc++;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) { dwSizeImage = 0; }

				GlobalUnlock(hDib);

				g_hDibThumb = hDib;
				g_hBitmapThumb = CreatePremultipliedBitmap(hDib);
			}

			// View the thumbnail immediately
			HWND hwndThumb = GetDlgItem(GetParent(hwndCtl), IDC_THUMB);
			if (hwndThumb != NULL)
			{
				if (InvalidateRect(hwndThumb, NULL, FALSE))
					UpdateWindow(hwndThumb);
			}
		}

		GlobalFreePtr(lpData);

		return TRUE;
	}

	// Version
	if (!ReadNat16(hFile, &wVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Version:\t%d"), wVersion);
	if (wVersion > 1) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Size of the following RIFF container
	DWORD dwImageSize = 0;
	if (!ReadNat32(hFile, &dwImageSize))
		return FALSE;

	if (FormatByteSize(dwImageSize, szOutput, _countof(szOutput)))
	{
		OutputText(hwndCtl, TEXT("Image Size:\t"));
		OutputText(hwndCtl, szOutput);
		OutputText(hwndCtl, g_szCRLF);
	}

	if (dwImageSize == 0)
		return TRUE;

	// Read and display the WebP image
	LPVOID lpData = GlobalAllocPtr(GHND, dwImageSize);
	if (lpData == NULL)
		return FALSE;

	if (!ReadData(hFile, lpData, dwImageSize))
	{
		GlobalFreePtr(lpData);
		return FALSE;
	}

	// Decode the thumbnail image
	HANDLE hDib = NULL;
	__try { hDib = WebpToDib(lpData, dwImageSize); }
	__except (EXCEPTION_EXECUTE_HANDLER) { hDib = NULL; }
	if (hDib != NULL)
	{
		if (g_hBitmapThumb != NULL)
			FreeBitmap(g_hBitmapThumb);
		if (g_hDibThumb != NULL)
			FreeDib(g_hDibThumb);

		g_hDibThumb = hDib;
		g_hBitmapThumb = CreatePremultipliedBitmap(hDib);

		// View the thumbnail immediately
		HWND hwndThumb = GetDlgItem(GetParent(hwndCtl), IDC_THUMB);
		if (hwndThumb != NULL)
		{
			if (InvalidateRect(hwndThumb, NULL, FALSE))
				UpdateWindow(hwndThumb);
		}
	}
	else
	{
		// Draw "UNSUPPORTED FORMAT" lettering over the default thumbnail image
		HWND hwndThumb = GetDlgItem(GetParent(hwndCtl), IDC_THUMB);
		if (hwndThumb != NULL)
		{
			if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_UNSUPPORTED : IDS_ENG_UNSUPPORTED,
				szOutput, _countof(szOutput)) > 0)
			{
				SetWindowText(hwndThumb, szOutput);
				if (InvalidateRect(hwndThumb, NULL, FALSE))
					UpdateWindow(hwndThumb);
			}
		}
	}

	GlobalFreePtr(lpData);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CollectorTimeChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckTime)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckTime->dwId);

	// Jump to Time chunk
	if (!FileSeekBegin(hFile, pckTime->dwOffset))
		return FALSE;

	// Lightmap Cache timestamp
	FILETIME ftTime = {0};
	SYSTEMTIME stTime = {0};

	// Timestamp
	if (!ReadData(hFile, (LPVOID)&ftTime, 8))
		return FALSE;

	OutputText(hwndCtl, TEXT("Timestamp:\t"));
	if (ftTime.dwLowDateTime == 0 && ftTime.dwHighDateTime == 0)
		OutputText(hwndCtl, TEXT("0"));
	else if (FileTimeToSystemTime(&ftTime, &stTime))
		OutputTextFmt(hwndCtl, szOutput, TEXT("%02u-%02u-%02u %02u:%02u:%02u"),
			stTime.wYear, stTime.wMonth, stTime.wDay, stTime.wHour, stTime.wMinute, stTime.wSecond);
	else
		OutputTextFmt(hwndCtl, szOutput, TEXT("%08X%08X"), ftTime.dwHighDateTime, ftTime.dwLowDateTime);
	OutputText(hwndCtl, g_szCRLF);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CollectorSkinChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckSkin)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckSkin->dwId);

	// Jump to Path chunk
	if (!FileSeekBegin(hFile, pckSkin->dwOffset))
		return FALSE;

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 0) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Root path to default skin
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Default Skin:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ObjectInfoTypeChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckType)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckType->dwId);

	// Jump to Type chunk
	if (!FileSeekBegin(hFile, pckType->dwOffset))
		return FALSE;

	// Type
	DWORD dwType = UNASSIGNED;
	if (!ReadNat32(hFile, &dwType))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("%s Type:\t"),
		pckType->dwId == 0x2E002000 ? TEXT("Item") : TEXT("Object"));
	switch (dwType)
	{
		case 0:
			OutputText(hwndCtl, TEXT("Undefined"));
			break;
		case 1:
			OutputText(hwndCtl, TEXT("Ornament"));
			break;
		case 2:
			OutputText(hwndCtl, TEXT("PickUp"));
			break;
		case 3:
			OutputText(hwndCtl, TEXT("Character"));
			break;
		case 4:
			OutputText(hwndCtl, TEXT("Vehicle"));
			break;
		case 5:
			OutputText(hwndCtl, TEXT("Spot"));
			break;
		case 6:
			OutputText(hwndCtl, TEXT("Cannon"));
			break;
		case 7:
			OutputText(hwndCtl, TEXT("Group"));
			break;
		case 8:
			OutputText(hwndCtl, TEXT("Decal"));
			break;
		case 9:
			OutputText(hwndCtl, TEXT("Turret"));
			break;
		case 10:
			OutputText(hwndCtl, TEXT("Wagon"));
			break;
		case 11:
			OutputText(hwndCtl, TEXT("Block"));
			break;
		case 12:
			OutputText(hwndCtl, TEXT("EntitySpawner"));
			break;
		case 13:
			OutputText(hwndCtl, TEXT("DeprecV"));
			break;
		case 14:
			OutputText(hwndCtl, TEXT("Procedural"));
			break;
		case UNASSIGNED:
			OutputText(hwndCtl, g_szUnknown);
			break;
		default:
			OutputTextFmt(hwndCtl, szOutput, TEXT("%d"), dwType);
	}
	OutputText(hwndCtl, g_szCRLF);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ObjectInfoVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckVersion)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckVersion->dwId);

	// Jump to Version chunk
	if (!FileSeekBegin(hFile, pckVersion->dwOffset))
		return FALSE;

	// Version
	DWORD dwVersion = 0;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Version:\t%d\r\n"), dwVersion);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DecorationMoodChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckMood)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckMood->dwId);

	// Jump to the Mood Remaping chunk
	if (!FileSeekBegin(hFile, pckMood->dwOffset))
		return FALSE;

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 0) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Mood Remaping
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, g_szFolder);
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	return ReadSkin(hwndCtl, hFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DecorationUnknownChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckUnknown)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckUnknown->dwId);

	// Jump to unknown chunk
	if (!FileSeekBegin(hFile, pckUnknown->dwOffset))
		return FALSE;

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 0) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Unknown
	DWORD dwUnknown = 0;
	if (!ReadNat32(hFile, &dwUnknown))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Unknown:\t%d\r\n"), dwUnknown);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CollectionOldDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckOldDesc)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	IDENTIFIER id;
	ResetIdentifier(&id);

	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckOldDesc->dwId);

	// Jump to Old Desc chunk
	if (!FileSeekBegin(hFile, pckOldDesc->dwOffset))
		return FALSE;

	// Environment
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Collection:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CollectionDescChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckDesc)
{
	FLOAT fVec2X, fVec2Y;
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	IDENTIFIER id;
	ResetIdentifier(&id);

	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckDesc->dwId);

	// Jump to Desc chunk
	if (!FileSeekBegin(hFile, pckDesc->dwOffset))
		return FALSE;

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 10) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Collection
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Collection:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Need Unlock
	BOOL bNeedUnlock = FALSE;
	if (!ReadBool(hFile, &bNeedUnlock))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Need Unlock:\t%s\r\n"),
		bNeedUnlock ? g_szTrue : g_szFalse);

	if (cVersion < 1)
		return TRUE;

	// Icon Env
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Env Icon:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Icon Collection
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Col Icon:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 2)
		return TRUE;

	// Sort Index
	int nSortIndex = 0;
	if (!ReadInteger(hFile, &nSortIndex))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Sort Index:\t%d\r\n"), nSortIndex);

	if (cVersion < 3)
		return TRUE;

	// Default Zone
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Default Zone:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 4)
		return TRUE;

	// Vehicle
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Vehicle:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Collection
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Collection:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Autor Name
	if ((nRet = ReadIdentifier(hFile, &id, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Author:\t\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 5)
		return TRUE;

	// Map Fid
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Map:\t\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Skip four unknown Real variables
	if (!FileSeekCurrent(hFile, 16))
		return FALSE;

	if (cVersion <= 7)
	{
		// MapCoordElem
		if (!ReadReal(hFile, &fVec2X) || !ReadReal(hFile, &fVec2Y))
			return FALSE;

		_sntprintf(szOutput, _countof(szOutput), TEXT("Map Coord Elem:\t(%g, %g)\r\n"),
			(double)fVec2X, (double)fVec2Y);
		OutputText(hwndCtl, szOutput);

		if (cVersion >= 6)
		{
			// MapCoordIcon
			if (!ReadReal(hFile, &fVec2X) || !ReadReal(hFile, &fVec2Y))
				return FALSE;

			_sntprintf(szOutput, _countof(szOutput), TEXT("Map Coord Icon:\t(%g, %g)\r\n"),
				(double)fVec2X, (double)fVec2Y);
			OutputText(hwndCtl, szOutput);
		}
	}

	if (cVersion < 7)
		return TRUE;

	// Load Screen
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Load Screen:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 8)
		return TRUE;

	// MapCoordElem
	if (!ReadReal(hFile, &fVec2X) || !ReadReal(hFile, &fVec2Y))
		return FALSE;

	_sntprintf(szOutput, _countof(szOutput), TEXT("Map Coord Elem:\t(%g, %g)\r\n"),
		(double)fVec2X, (double)fVec2Y);
	OutputText(hwndCtl, szOutput);

	// MapCoordIcon
	if (!ReadReal(hFile, &fVec2X) || !ReadReal(hFile, &fVec2Y))
		return FALSE;

	_sntprintf(szOutput, _countof(szOutput), TEXT("Map Coord Icon:\t(%g, %g)\r\n"),
		(double)fVec2X, (double)fVec2Y);
	OutputText(hwndCtl, szOutput);

	// MapCoordDesc
	if (!ReadReal(hFile, &fVec2X) || !ReadReal(hFile, &fVec2Y))
		return FALSE;

	_sntprintf(szOutput, _countof(szOutput), TEXT("Map Coord Desc:\t(%g, %g)\r\n"),
		(double)fVec2X, (double)fVec2Y);
	OutputText(hwndCtl, szOutput);

	// Long Desc
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Long Desc:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 9)
		return TRUE;

	// Display Name
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Display Name:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 10)
		return TRUE;

	// Is Editable
	BOOL bIsEditable = FALSE;
	if (!ReadBool(hFile, &bIsEditable))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Is Editable:\t%s\r\n"),
		bIsEditable ? g_szTrue : g_szFalse);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CollectionFoldersChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckFolders)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckFolders->dwId);

	// Jump to Collector Folders chunk
	if (!FileSeekBegin(hFile, pckFolders->dwOffset))
		return FALSE;

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 4) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Folder Block Info
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Block Info:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Folder Item
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Item Info:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Folder Decoration
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Decoration:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion >= 1 && cVersion <= 2)
	{
		// Folder
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, g_szFolder);
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	if (cVersion < 2)
		return TRUE;

	// Folder Card Event Info
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Card Event:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 3)
		return TRUE;

	// Folder Macro Block Info
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Macro Block:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	if (cVersion < 4)
		return TRUE;

	// Folder Macro Decals
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Macro Decals:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CollectionMenuIconsChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckMenuIcons)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckMenuIcons->dwId);

	// Jump to the Menu Icons Folders chunk
	if (!FileSeekBegin(hFile, pckMenuIcons->dwOffset))
		return FALSE;

	// Chunk version
	BYTE cVersion = 0;
	if (!ReadNat8(hFile, &cVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, (char)cVersion);
	if (cVersion > 0) OutputText(hwndCtl, g_szAsterisk);
	OutputText(hwndCtl, g_szCRLF);

	// Folder Menus Icons
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Menus Icons:\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL PlugVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckVersion)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckVersion->dwId);

	// Jump to Version chunk
	if (!FileSeekBegin(hFile, pckVersion->dwOffset))
		return FALSE;

	// Version
	DWORD dwVersion = 0;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, dwVersion);
	OutputText(hwndCtl, g_szCRLF);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL HmsVersionChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckVersion)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckVersion->dwId);

	// Jump to Version chunk
	if (!FileSeekBegin(hFile, pckVersion->dwOffset))
		return FALSE;

	// Version
	DWORD dwVersion = 0;
	if (!ReadNat32(hFile, &dwVersion))
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, g_szVersion, dwVersion);
	OutputText(hwndCtl, g_szCRLF);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL GameSkinChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckGameSkin)
{
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckGameSkin->dwId);

	// Jump to Skin chunk
	if (!FileSeekBegin(hFile, pckGameSkin->dwOffset))
		return FALSE;

	return ReadSkin(hwndCtl, hFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL ProfileChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckProfile)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckProfile->dwId);

	// Jump to the Net Player Profile chunk
	if (!FileSeekBegin(hFile, pckProfile->dwOffset))
		return FALSE;

	// Online Login
	if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
		return FALSE;

	if (nRet > 0)
	{
		OutputText(hwndCtl, TEXT("Login:\t\t"));
		ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
		OutputText(hwndCtl, szOutput);
	}

	// Length of the Online Support Key
	DWORD dwLen = 0;
	if (!ReadNat32(hFile, &dwLen) || dwLen > 0xFFF)
		return FALSE;

	if (dwLen > 0)
	{
		LPVOID lpData = GlobalAllocPtr(GHND, (SIZE_T)dwLen + 1);
		if (lpData == NULL)
			return FALSE;

		// Online Support Key
		if (!ReadData(hFile, lpData, dwLen))
		{
			GlobalFreePtr(lpData);
			return FALSE;
		}

		DWORD dwCount = dwLen;
		LPBYTE lpByte = (LPBYTE)lpData;
		BOOL bHexDigits = TRUE;

		while (dwCount--)
		{
			char ch = *lpByte++;
			if ((ch < '0' || ch > '9') &&
				(ch < 'A' || ch > 'F') &&
				(ch < 'a' || ch > 'f'))
			{
				bHexDigits = FALSE;
				break;
			}
		}

		if (dwLen >= 32 && bHexDigits) // MP
			MultiByteToWideChar(CP_OEMCP, MB_USEGLYPHCHARS,
				(LPCSTR)lpData, dwLen+1, szOutput, _countof(szOutput) - 1);
		else
		{ // TMF
			lpByte = (LPBYTE)lpData;
			dwCount = min(dwLen, _countof(szOutput)/2 - 1);
			szOutput[0] = g_chNil;
			while (dwCount--)
				_sntprintf(szOutput, _countof(szOutput), TEXT("%s%02X"), (LPTSTR)szOutput, (WORD)*lpByte++);
		}

		OutputText(hwndCtl, TEXT("Support Key:\t"));
		OutputText(hwndCtl, szOutput);
		OutputText(hwndCtl, g_szCRLF);

		GlobalFreePtr(lpData);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOOL FolderDepChunk(HWND hwndCtl, HANDLE hFile, PCHUNK pckFolder)
{
	SSIZE_T nRet = 0;
	CHAR szRead[ID_LEN];
	TCHAR szOutput[OUTPUT_LEN];
	OutputTextFmt(hwndCtl, szOutput, g_szSep3, pckFolder->dwId);

	// Jump to Nod chunk
	if (!FileSeekBegin(hFile, pckFolder->dwOffset))
		return FALSE;

	// Number of entries
	DWORD dwCount = 0;
	if (!ReadNat32(hFile, &dwCount) || dwCount > 0xFF)
		return FALSE;

	OutputTextFmt(hwndCtl, szOutput, TEXT("Number:\t\t%d\r\n"), dwCount);

	while (dwCount--)
	{
		// Folder Dep
		if ((nRet = ReadString(hFile, szRead, _countof(szRead))) < 0)
			return FALSE;

		if (nRet > 0)
		{
			OutputText(hwndCtl, g_szFolder);
			ConvertGbxString(szRead, nRet, szOutput, _countof(szOutput));
			OutputText(hwndCtl, szOutput);
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
