// MQ2Posse.cpp : RedGuides exclusive plugin to manage commands when friends and strangers are nearby
// v1.00 :: Sym - 2012-09-27
// v1.01 :: Sym - 2012-09-30
// v1.02 :: Sym - 2012-12-15
// v1.03 :: EqMule - 2015-03-09
// v1.04 :: Ctaylor22 - 2016-07-22
// v1.05 :: Sym - 2017-05-04
// v1.06 :: Plure - 2017-05-04
// v1.07 :: Sym - 2017-07-29 - Added Posse.StrangerNames TLO. Space separated list of current strangers.
// v1.08 :: EqMule - 2018-10-18 - Fixed support for taking guild members into account
// v1.09 :: Plure - 2019-07-04 - White listed people in your group and those connected to mq2dannet, added Posse.FriendNames TLO, 
//			made it so if someone went from being a stranger to friend or friend to stranger it would recognized this, 
//			added the ability to ignore people in your fellowship, fixed a bug where if you set it to ignore your guild and at some point you became guildless it would ignore everyone not in a guild.

#include "../MQ2Plugin.h"
using namespace std;
#include <vector>
#include "mmsystem.h"
#pragma comment(lib, "winmm.lib")

PLUGIN_VERSION(1.09);

//#pragma warning(disable:4244)
PreSetup("MQ2Posse");

#define SKIP_PULSES 50
vector <string> vSeen;
vector <string> vGlobalNames;
vector <string> vIniNames;
vector <string> vNames;
vector <string> vCommands;
vector <string> vFriends;
vector <string> vStrangers;
SEARCHSPAWN mySpawn;

bool bPosseEnabled = false;
bool bInitDone = false;
bool bIgnoreFellowship = false;
bool bIgnoreGuild = false;
bool bNotify = true;
bool bAudio = false;
bool bNotifyFriends = true;
bool bNotifyStrangers = true;
bool bDebug = false;

unsigned int gAudioTimer=0;
unsigned int gAudioDelay=30000;

long SkipPulse = 0;
int SpawnRadius = 300;
int SpawnZRadius = 30;
char szSkipZones[MAX_STRING];
char PosseSound[MAX_STRING];

class MQ2PosseType *pPosseType = 0;
class MQ2PosseType : public MQ2Type
{
public:
	enum PosseMembers {
		Status = 1,
		Count = 2,
		Radius = 3,
		ZRadius = 4,
		Friends = 5,
		FriendNames = 6,
		Strangers = 7,
		StrangerNames = 8
	};

	MQ2PosseType() :MQ2Type("Posse")
	{
		TypeMember(Status);
		TypeMember(Count);
		TypeMember(Radius);
		TypeMember(ZRadius);
		TypeMember(Friends);
		TypeMember(FriendNames);
		TypeMember(Strangers);
		TypeMember(StrangerNames);
	}

	~MQ2PosseType()
	{
	}

	bool GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR &Dest)
	{
		PMQ2TYPEMEMBER pMember = MQ2PosseType::FindMember(Member);
		if (!pMember)
			return false;
		switch ((PosseMembers)pMember->ID)
		{
		case Status:
			Dest.Int = bPosseEnabled;
			Dest.Type = pBoolType;
			return true;
		case Count:
			Dest.Int = vSeen.size();
			Dest.Type = pIntType;
			return true;
		case Radius:
			Dest.Int = SpawnRadius;
			Dest.Type = pIntType;
			return true;
		case ZRadius:
			Dest.Int = SpawnZRadius;
			Dest.Type = pIntType;
			return true;
		case Friends:
			Dest.Int = vFriends.size();
			Dest.Type = pIntType;
			return true;
		case FriendNames:
			if (vFriends.size()) 
			{
				char szTemp[MAX_STRING];
				char MQ2PosseTypeTemp[MAX_STRING];
				for (register unsigned int a = 0; a < vFriends.size(); a++) {
					string& vRef = vFriends[a];
					sprintf_s(szTemp, "%s ", vRef.c_str());
					strcat_s(MQ2PosseTypeTemp, szTemp);
				}
				strcpy_s(DataTypeTemp, MQ2PosseTypeTemp);
				Dest.Ptr = &DataTypeTemp[0];
				Dest.Type = pStringType;
			}
			else 
			{
				strcpy_s(DataTypeTemp, MAX_STRING, "NONE");
				Dest.Ptr = &DataTypeTemp[0];
				Dest.Type = pStringType;
			}
			return true;
		case Strangers:
			Dest.Int = vStrangers.size();
			Dest.Type = pIntType;
			return true;
		case StrangerNames:
			if (vStrangers.size()) 
			{
				char szTemp[MAX_STRING];
				char MQ2PosseTypeTemp[MAX_STRING];
				for (register unsigned int a = 0; a < vStrangers.size(); a++) 
				{
					string& vRef = vStrangers[a];
					sprintf_s(szTemp, "%s ", vRef.c_str());
					strcat_s(MQ2PosseTypeTemp, szTemp);
				}
				strcpy_s(DataTypeTemp, MQ2PosseTypeTemp);
				Dest.Ptr = &DataTypeTemp[0];
				Dest.Type = pStringType;
			}
			else {
				strcpy_s(DataTypeTemp, MAX_STRING, "NONE");
				Dest.Ptr = &DataTypeTemp[0];
				Dest.Type = pStringType;
			}
			return true;
		}
		return false;
	}

	bool ToString(MQ2VARPTR VarPtr, PCHAR Destination)
	{
		strcpy_s(Destination, MAX_STRING, "FALSE");
		if (bPosseEnabled)
			strcpy_s(Destination, MAX_STRING, "TRUE");
		return true;
	}

	bool FromData(MQ2VARPTR &VarPtr, MQ2TYPEVAR &Source)
	{
		return false;
	}
	bool FromString(MQ2VARPTR &VarPtr, PCHAR Source)
	{
		return false;
	}
};

BOOL dataPosse(PCHAR szName, MQ2TYPEVAR &Ret)
{
    Ret.DWord=1;
    Ret.Type=pPosseType;
    return true;
}

void ListUsers() 
{
    WriteChatf("User list contains \ay%d\ax %s", vNames.size(), vNames.size() > 1 ? "entries" : "entry");
    for (unsigned int a = 0; a < vNames.size(); a++)
	{
        string& vRef = vNames[a];
        WriteChatf("\at%d: %s\ax", a+1, vRef.c_str());
    }
}

void ListCommands() 
{
    WriteChatf("Command list contains \ay%d\ax %s", vCommands.size(), vCommands.size() > 1 ? "entries" : "entry");
    for (unsigned int a = 0; a < vCommands.size(); a++) 
	{
        string& vRef = vCommands[a];
        WriteChatf("\at%d: %s\ax", a+1, vRef.c_str());
    }
}

void CombineNames() 
{
    vNames.clear();
    for (register unsigned int a = 0; a < vGlobalNames.size(); a++) 
	{
        string& vRef = vGlobalNames[a];
        vNames.push_back(vRef.c_str());
    }
    for (register unsigned int a = 0; a < vIniNames.size(); a++) 
	{
        string& vRef = vIniNames[a];
        vNames.push_back(vRef.c_str());
    }
}


void Update_INIFileName() 
{
    sprintf_s(INIFileName,"%s\\MQ2Posse.ini",gszINIPath);
}

void ShowStatus(void) 
{
    WriteChatf("\atMQ2Posse :: v%1.2f :: by Sym for RedGuides.com\ax", MQ2Version);
    WriteChatf("MQ2Posse :: %s", bPosseEnabled?"\agENABLED\ax":"\arDISABLED\ax");
    WriteChatf("MQ2Posse :: Radius is \ag%d\ax", SpawnRadius);
    WriteChatf("MQ2Posse :: ZRadius is \ag%d\ax", SpawnZRadius);
	WriteChatf("MQ2Posse :: Ignore people in fellowship is %s", bIgnoreFellowship ? "\agON\ax" : "\arOFF\ax");
    WriteChatf("MQ2Posse :: Ignore guildmates is %s", bIgnoreGuild ? "\agON\ax" : "\arOFF\ax");
    WriteChatf("MQ2Posse :: Notfications are %s", bNotify ? "\agON\ax" : "\arOFF\ax");
    WriteChatf("MQ2Posse :: Friend notfications are %s", bNotifyFriends ? "\agON\ax" : "\arOFF\ax");
    WriteChatf("MQ2Posse :: Stranger notfications are %s", bNotifyStrangers ? "\agON\ax" : "\arOFF\ax");
    WriteChatf("MQ2Posse :: Audio alerts are %s", bAudio ? "\agON\ax" : "\arOFF\ax");
    WriteChatf("MQ2Posse :: Zones to skip: \ag%s\ax",szSkipZones);
    CombineNames();
	if (vNames.size())
	{
		ListUsers();
	}
	if (vCommands.size())
	{
		ListCommands();
	}
}

VOID LoadINI(VOID) 
{
    Update_INIFileName();
    // get on/off settings
	char szTemp[MAX_STRING];
	char szWavePath[MAX_STRING];
	char szList[MAX_STRING];
    sprintf_s(szTemp,"%s_Settings",GetCharInfo()->Name);
    bPosseEnabled = GetPrivateProfileInt(szTemp, "Enabled", 0, INIFileName) > 0 ? true : false;
    SpawnRadius = GetPrivateProfileInt(szTemp, "Radius", 300, INIFileName);
    SpawnZRadius = GetPrivateProfileInt(szTemp, "ZRadius", 30, INIFileName);
    bNotify = GetPrivateProfileInt(szTemp, "Notify", 1, INIFileName) > 0 ? true : false;
    bNotifyFriends = GetPrivateProfileInt(szTemp, "NotifyFriends", 1, INIFileName) > 0 ? true : false;
    bNotifyStrangers = GetPrivateProfileInt(szTemp, "NotifyStrangers", 1, INIFileName) > 0 ? true : false;
	bIgnoreFellowship = GetPrivateProfileInt(szTemp, "IgnoreFellowship", 0, INIFileName) > 0 ? true : false;
    bIgnoreGuild = GetPrivateProfileInt(szTemp, "IgnoreGuild", 0, INIFileName) > 0 ? true : false;
    bAudio = GetPrivateProfileInt(szTemp, "Audio", 0, INIFileName) > 0 ? true : false;
    sprintf_s(szWavePath,"%s\\mq2posse.wav",gszINIPath);
    GetPrivateProfileString(szTemp,"Soundfile", szWavePath,PosseSound,2047,INIFileName);
    GetPrivateProfileString(szTemp,"SkipZones","poknowledge,guildhall,guildlobby,bazaar,neighborhood",szSkipZones,2047,INIFileName);

    // get all names
    sprintf_s(szTemp,"%s_Names",GetCharInfo()->Name);
    GetPrivateProfileSection(szTemp,szList,MAX_STRING,INIFileName);

    // clear list
    vIniNames.clear();

    char* p = (char*)szList;
    char *pch = 0;
    char *next_pch = 0;
    size_t length = 0;
    int i = 0;
    // loop through all entries under _Names
    // values are terminated by \0, final value is teminated with \0\0
    // values look like
    // Charactername=1
    while (*p)
    {
        length = strlen(p);
        // split entries on =
        pch = strtok_s(p, "=", &next_pch);
        if (pch != NULL)
        {
            // Odd entries are the names. Add it to the list
            vIniNames.push_back(pch);

            // next is value. Don't use it so skip it
            //pch = strtok_s(NULL, "=", &next_pch);

            // next name
            //pch = strtok_s(NULL, "=", &next_pch);
            i++;
        }
        p += length;
        p++;
    }

    sprintf_s(szTemp,"%s_Commands",GetCharInfo()->Name);
    GetPrivateProfileSection(szTemp,szList,MAX_STRING,INIFileName);

    // clear list
    vCommands.clear();

    p = (char*)szList;
    length = 0;
    i = 0;
    // loop through all entries under _Commands
    // values are terminated by \0, final value is teminated with \0\0
    // values look like
    // #=\command
    while (*p)
    {
        length = strlen(p);
        // split entries on =
        pch = strtok_s(p, "=", &next_pch);
        if (pch != NULL)
        {
            // Don't use key so skip it
            pch = strtok_s(NULL, "=", &next_pch);

            // Even entries are the commands. Add it to the list
            vCommands.push_back(pch);

            // next key
            //pch = strtok_s(NULL, "=", &next_pch);
            i++;
        }
        p += length;
        p++;
    }

    GetPrivateProfileSection("GlobalNames",szList,MAX_STRING,INIFileName);
    vGlobalNames.clear();
    p = (char*)szList;
    length = 0;
    i = 0;
    // loop through all entries under _Names
    // values are terminated by \0, final value is teminated with \0\0
    // values look like
    // Charactername=1
    while (*p)
    {
        length = strlen(p);
        // split entries on =
        pch = strtok_s(p, "=", &next_pch);
        if (pch != NULL)
        {
            // Odd entries are the names. Add it to the list
            vGlobalNames.push_back(pch);

            // next is value. Don't use it so skip it
            //pch = strtok_s(NULL, "=", &next_pch);

            // next name
            //pch = strtok_s(NULL, "=", &next_pch);
            i++;
        }
        p += length;
        p++;
    }

    // set seach conditions pc range 0 100 radius 300 zradius 300
    ClearSearchSpawn(&mySpawn);
    mySpawn.SpawnType = PC;
    mySpawn.MaxLevel = MAX_PC_LEVEL;
    mySpawn.GuildID = -1;
    mySpawn.ZRadius = SpawnZRadius;
    mySpawn.FRadius = SpawnRadius;

    ShowStatus();
    // flag first load init as done
    bInitDone = true;
}

VOID SaveINI(VOID) {
    char ToStr[16];

    Update_INIFileName();
    // write on/off settings
	char szTemp[MAX_STRING];
    sprintf_s(szTemp,"%s_Settings",GetCharInfo()->Name);
    WritePrivateProfileSection(szTemp, "", INIFileName);
    WritePrivateProfileString(szTemp,"Enabled",bPosseEnabled? "1" : "0",INIFileName);
    sprintf_s(ToStr,"%d",SpawnRadius);
    WritePrivateProfileString(szTemp,"Radius",ToStr,INIFileName);
    sprintf_s(ToStr,"%d",SpawnZRadius);
    WritePrivateProfileString(szTemp,"ZRadius",ToStr,INIFileName);
    WritePrivateProfileString(szTemp,"Soundfile",PosseSound,INIFileName);
    WritePrivateProfileString(szTemp,"Audio",bAudio? "1" : "0",INIFileName);
	WritePrivateProfileString(szTemp,"IgnoreFellowship", bIgnoreFellowship ? "1" : "0", INIFileName);
    WritePrivateProfileString(szTemp,"IgnoreGuild",bIgnoreGuild? "1" : "0",INIFileName);
    WritePrivateProfileString(szTemp,"Notify",bNotify? "1" : "0",INIFileName);
    WritePrivateProfileString(szTemp,"NotifyFriends",bNotifyFriends? "1" : "0",INIFileName);
    WritePrivateProfileString(szTemp,"NotifyStrangers",bNotifyStrangers? "1" : "0",INIFileName);
    WritePrivateProfileString(szTemp,"SkipZones",szSkipZones,INIFileName);

    // write all names
    char szCount[MAX_STRING];
    sprintf_s(szTemp,"%s_Names",GetCharInfo()->Name);
    WritePrivateProfileSection(szTemp, "", INIFileName);
    for (unsigned int a = 0; a < vIniNames.size(); a++) {
        string& vRef = vIniNames[a];
        WritePrivateProfileString(szTemp,vRef.c_str(),"1",INIFileName);
    }
    sprintf_s(szTemp,"%s_Commands",GetCharInfo()->Name);
    WritePrivateProfileSection(szTemp, "", INIFileName);
    for (unsigned int a = 0; a < vCommands.size(); a++) {
        string& vRef = vCommands[a];
        sprintf_s(szCount,"%d",a);
        WritePrivateProfileString(szTemp,szCount,vRef.c_str(),INIFileName);
    }

    WriteChatf("MQ2Posse :: Settings updated");
    LoadINI();
}

void ShowHelp(void) {
    WriteChatf("\atMQ2Posse :: v%1.2f :: by Sym for RedGuides.com\ax", MQ2Version);
    WriteChatf("/posse :: Lists command syntax");
    WriteChatf("/posse on|off :: Toggles checking of new pc's. Default is \arOFF\ax");
    WriteChatf("/posse status :: Shows the current status and settings");
    WriteChatf("/posse load :: Loads ini file.");
    WriteChatf("/posse save :: Saves settings to ini. Changes \arDO NOT\ax auto save.");
    WriteChatf("/posse add NAME :: Adds NAME to friendly list.");
    WriteChatf("/posse del NAME :: Deletes NAME from friendly list");
    WriteChatf("/posse clear :: Clears all char names.");
    WriteChatf("/posse radius # :: Sets check radius to # (ex: /posse radius 300).");
    WriteChatf("/posse zradius # :: Sets check zradius to # (ex: /posse zradius 30).");
    WriteChatf("/posse list :: Lists current names and commands.");
    WriteChatf("/posse notify on|off :: Toggles notification alerts. Default is \agON\ax.");
    WriteChatf("/posse friendnotify on|off :: Toggles notification alerts for friends. Default is \agON\ax.");
    WriteChatf("/posse strangernotify on|off :: Toggles notification alerts for strangers. Default is \agON\ax.");
    WriteChatf("/posse guild on|off :: Toggles ignoring guild members, meaning guild members are invisible to all checks. Default is \arOFF\ax.");
	WriteChatf("/posse fellowship on|off :: Toggles ignoring fellowship members, meaning fellowship members are invisible to all checks. Default is \arOFF\ax.");
    WriteChatf("/posse audio on|off :: Toggles audio alerts for strangers. Default is \arOFF\ax.");
    WriteChatf("/posse testaudio :: Test plays the audio alert.");
    WriteChatf("/posse cmdadd \"COMMAND\" :: Adds COMMAND that is executed when a stranger is near");
    WriteChatf("/posse cmddel # :: Deletes a command that is executed when a stranger is near");
    WriteChatf(" ");
    WriteChatf("\atTLOs\ax");
    WriteChatf("\ay${Posse.Status}\ax :: Returns enabled status as true/false");
    WriteChatf("\ay${Posse.Radius}\ax :: Returns size of check radius");
    WriteChatf("\ay${Posse.ZRadius}\ax :: Returns size of check zradius");
    WriteChatf("\ay${Posse.Count}\ax :: Returns how many total people in radius");
    WriteChatf("\ay${Posse.Friends}\ax :: Returns how many friends are in radius");
	WriteChatf("\ay${Posse.FriendNames}\ax :: Returns space delimited list of friends names in radius");
    WriteChatf("\ay${Posse.Strangers}\ax :: Returns how many strangers are in radius");
    WriteChatf("\ay${Posse.StrangerNames}\ax :: Returns space delimited list of strangers names in radius");
}

VOID doCommands(VOID) {
    PSPAWNINFO pChar = GetCharInfo()->pSpawn;
    for (register unsigned int a = 0; a < vCommands.size(); a++) {
        string& vRef = vCommands[a];
        DoCommand(pChar, (PCHAR)vRef.c_str());
    }
}

VOID PosseCommand(PSPAWNINFO pChar, PCHAR zLine) {
    char szTemp[MAX_STRING], szBuffer[MAX_STRING];

    GetArg(szTemp, zLine, 1);
    if (!_strnicmp(szTemp,"testaudio",9)) {
        WriteChatf("MQ2Posse :: \agTesting \atAudio \ayAlert\ax");
        PlaySound(PosseSound,0,SND_ASYNC);
        return;
    }
    if(!_strnicmp(szTemp,"load",4)) {
        LoadINI();
        return;
    }
    if(!_strnicmp(szTemp,"save",4)) {
        SaveINI();
        return;
    }
    if(!_strnicmp(szTemp,"on",2)) {
        bPosseEnabled = true;
        WriteChatf("MQ2Posse :: \agEnabled\ax");
    }
    else if(!_strnicmp(szTemp,"off",2)) {
        bPosseEnabled = false;
		vFriends.clear();
		vStrangers.clear();
		vSeen.clear();
        WriteChatf("MQ2Posse :: \arDisabled\ax");
    }
    else if (!_strnicmp(szTemp, "status",6)) {
        ShowStatus();
    }
    else if (!_strnicmp(szTemp, "radius",6)) {
        GetArg(szBuffer, zLine, 2);
        if(strlen(szBuffer)>0) {
            SpawnRadius = atoi(szBuffer);
            mySpawn.FRadius = SpawnRadius;
            WriteChatf("MQ2Posse :: Radius is \ag%d\ax", SpawnRadius);
        } else {
            WriteChatf("MQ2Posse :: Radius not specified.");
        }
    }
    else if (!_strnicmp(szTemp, "zradius",7)) {
        GetArg(szBuffer, zLine, 2);
        if(strlen(szBuffer)>0) {
            SpawnZRadius = atoi(szBuffer);
            mySpawn.ZRadius = SpawnZRadius;
            WriteChatf("MQ2Posse :: ZRadius is \ag%d\ax", SpawnZRadius);
        } else {
            WriteChatf("MQ2Posse :: ZRadius not specified.");
        }
    }
    else if(!_strnicmp(szTemp,"add",3)) {
        GetArg(szTemp,zLine,2);
        if(!_stricmp(szTemp, "")) {
            WriteChatf("Usage: /posse add NAME");
            return;
        }
        for (unsigned int a = 0; a < vNames.size(); a++) {
            string& vRef = vNames[a];
            if (!_stricmp(szTemp,vRef.c_str())) {
                WriteChatf("MQ2Posse :: User \ay%s\ax already exists", szTemp);
                return;
            }
        }
        vIniNames.push_back(szTemp);
        CombineNames();
		vFriends.clear();
        vStrangers.clear();
        vSeen.clear();
        WriteChatf("MQ2Posse :: Added \ay%s\ax to name list", szTemp);
    }
    else if(!_strnicmp(szTemp,"del",3)) {
        int delIndex = -1;
        GetArg(szTemp,zLine,2);
        for (unsigned int a = 0; a < vIniNames.size(); a++) {
            string& vRef = vIniNames[a];
            if (!_stricmp(szTemp,vRef.c_str())) {
                delIndex = a;
            }
        }
        if (delIndex >= 0) {
            vIniNames.erase(vIniNames.begin() + delIndex);
            CombineNames();
			vFriends.clear();
            vStrangers.clear();
            vSeen.clear();
            WriteChatf("MQ2Posse :: Deleted user \ay%s\ax", szTemp);
        } else {
            WriteChatf("MQ2Posse :: User \ay%s\ax not found", szTemp);
        }
    }
    else if(!_strnicmp(szTemp,"cmdadd",7)) {
        GetArg(szTemp,zLine,2);
        if(!_stricmp(szTemp, "")) {
            WriteChatf("Usage: /posse cmdadd \"COMMAND\"");
            return;
        }
        for (unsigned int a = 0; a < vCommands.size(); a++) {
            string& vRef = vCommands[a];
            if (!_stricmp(szTemp,vRef.c_str())) {
                WriteChatf("MQ2Posse :: Command \ay%s\ax already exists", szTemp);
                return;
            }
        }
        vCommands.push_back(szTemp);
        WriteChatf("MQ2Posse :: Added \ay%s\ax to command list", szTemp);
    }
    else if (!_strnicmp(szTemp, "cmddel",6)) {
        GetArg(szBuffer, zLine, 2);
        unsigned int a = atoi(szBuffer);
        if(a > 0 && a < vCommands.size()+1) {
            string& vRef = vCommands[a-1];
            WriteChatf("MQ2Posse: Command \ay%d\ax \at%s\ax deleted.", a, vRef.c_str());
            vCommands.erase(vCommands.begin() + (a-1));
        } else {
            WriteChatf("MQ2Posse :: Command \ay%d\ax does not exist.", a);
        }
    }
    else if(!_strnicmp(szTemp,"notify",6)) {
        GetArg(szTemp,zLine,2);
        if(!_strnicmp(szTemp,"on",2)) {
            bNotify = true;
        } else if(!_strnicmp(szTemp,"off",2)) {
            bNotify = false;
        }
        WriteChatf("MQ2Posse :: Notfications are %s", bNotify ? "\agON\ax" : "\arOFF\ax");
    }
    else if(!_strnicmp(szTemp,"friendnotify",12)) {
        GetArg(szTemp,zLine,2);
        if(!_strnicmp(szTemp,"on",2)) {
            bNotifyFriends = true;
        } else if(!_strnicmp(szTemp,"off",2)) {
            bNotifyFriends = false;
        }
        WriteChatf("MQ2Posse :: Friend notfications are %s", bNotifyFriends ? "\agON\ax" : "\arOFF\ax");
    }
    else if(!_strnicmp(szTemp,"strangernotify",14)) {
        GetArg(szTemp,zLine,2);
        if(!_strnicmp(szTemp,"on",2)) {
            bNotifyStrangers = true;
        } else if(!_strnicmp(szTemp,"off",2)) {
            bNotifyStrangers = false;
        }
        WriteChatf("MQ2Posse :: Stranger notfications are %s", bNotifyStrangers ? "\agON\ax" : "\arOFF\ax");
    }
	else if (!_strnicmp(szTemp, "fellowship", 10)) {
		GetArg(szTemp, zLine, 2);
		if (!_strnicmp(szTemp, "on", 2)) {
			bIgnoreFellowship = true;
		}
		else if (!_strnicmp(szTemp, "off", 2)) {
			bIgnoreFellowship = false;
		}
		WriteChatf("MQ2Posse :: Ignore people in my fellowship is %s", bIgnoreFellowship ? "\agON\ax" : "\arOFF\ax");
	}
    else if(!_strnicmp(szTemp,"guild",5)) {
        GetArg(szTemp,zLine,2);
        if(!_strnicmp(szTemp,"on",2)) {
            bIgnoreGuild = true;
        } else if(!_strnicmp(szTemp,"off",2)) {
            bIgnoreGuild = false;
        }
        WriteChatf("MQ2Posse :: Ignore guildmates is %s", bIgnoreGuild ? "\agON\ax" : "\arOFF\ax");
    }
    else if(!_strnicmp(szTemp,"audio",5)) {
        GetArg(szTemp,zLine,2);
        if(!_strnicmp(szTemp,"on",2)) {
            bAudio = true;
        } else if(!_strnicmp(szTemp,"off",2)) {
            bAudio = false;
        }
        WriteChatf("MQ2Posse:: Audio alerts are %s", bAudio ? "\agON\ax" : "\arOFF\ax");
    }
    else if(!_strnicmp(szTemp,"list",4)) {
        ListUsers();
        ListCommands();
    }
    else if (!_strnicmp(szTemp, "debug", 5)) {
        GetArg(szTemp, zLine, 2);
        if (!_strnicmp(szTemp, "on", 2)) {
            bDebug = true;
        }
        else if (!_strnicmp(szTemp, "off", 2)) {
            bDebug = false;
        }
        WriteChatf("MQ2Posse :: Debugging is %s", bDebug ? "\agON\ax" : "\arOFF\ax");
    }
    else {
        ShowHelp();
    }
}

bool CheckGroup(PCHAR szName)
{
	if (PCHARINFO pChar = GetCharInfo())
	{
		if (pChar->pGroupInfo && pChar->pGroupInfo->pMember && pChar->pGroupInfo->pMember[0])
		{
			for (int a = 1; a < 6; a++) // Lets loop through the group and see if we can find this person in our
			{
				if (pChar->pGroupInfo->pMember[a])
				{
					if (pChar->pGroupInfo->pMember[a]->pName) // and we know their name
					{
						CHAR szGroupMemberName[MAX_STRING] = { 0 };
						GetCXStr(pChar->pGroupInfo->pMember[a]->pName, szGroupMemberName, MAX_STRING);
						if (!_stricmp(szGroupMemberName, szName)) // Ok so I am the ML
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

PMQPLUGIN Plugin(char* PluginName)
{
	long Length = strlen(PluginName) + 1;
	PMQPLUGIN pLook = pPlugins;
	while (pLook && _strnicmp(PluginName, pLook->szFilename, Length)) pLook = pLook->pNext;
	return pLook;
}

bool CheckEQBC(PCHAR szName)
{
	if (PMQPLUGIN pLook = Plugin("mq2eqbc"))
	{
		if (unsigned short(*fisConnected)() = (unsigned short(*)())GetProcAddress(pLook->hModule, "isConnected"))
		{
			if (fisConnected())
			{
				if (bool(*fAreTheyConnected)(char* szName) = (bool(*)(char* szName))GetProcAddress(pLook->hModule, "AreTheyConnected"))
				{
					if (fAreTheyConnected(szName))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool CheckMQ2DanNet(PCHAR szName)
{
	if (PMQPLUGIN pLook = Plugin("mq2dannet"))
	{
		if (bool(*f_peer_connected)(const std::string& name) = (bool(*)(const std::string& name))GetProcAddress(pLook->hModule, "peer_connected"))
		{
			char szTemp[MAX_STRING];
			sprintf_s(szTemp, "%s_%s", EQADDR_SERVERNAME, szName);
			if (f_peer_connected(szTemp))
			{
				return true;
			}
		}
	}
	return false;
}

void ClearFriendsAndStrangers(PCHAR szDeleteName)
{
	if (vFriends.size())
	{
		for (unsigned int a = 0; a < vFriends.size(); a++)
		{
			string& Ref = vFriends[a];
			if (!_stricmp(szDeleteName, Ref.c_str()))
			{
				vFriends.erase(vFriends.begin() + a);
				if (bNotify && bNotifyFriends)
				{
					WriteChatf("\ag%s has moved out of range.\ax", szDeleteName);
				}
			}
		}
	}
	if (vStrangers.size())
	{
		for (unsigned int a = 0; a < vStrangers.size(); a++)
		{
			string& Ref = vStrangers[a];
			if (!_stricmp(szDeleteName, Ref.c_str()))
			{
				vStrangers.erase(vStrangers.begin() + a);
				if (bNotify && bNotifyStrangers)
				{
					WriteChatf("\ar%s has moved out of range.\ax", szDeleteName);
				}
				if (bAudio) 
				{
					if (gAudioTimer <= MQGetTickCount64())
					{
						PlaySound(PosseSound, 0, SND_ASYNC);
						gAudioTimer = (unsigned int)MQGetTickCount64() + gAudioDelay;
					}
				}
			}
		}
	}
}

bool CheckNames(PCHAR szName) 
{
	CHAR szTemp[MAX_STRING];
	if (gGameState == GAMESTATE_INGAME) {
		if (pCharData) {
			strcpy_s(szTemp, szName);
			_strlwr_s(szTemp);
			// Lets check if they are on our white listed names
			for (register unsigned int a = 0; a < vNames.size(); a++)
			{
				string& vRef = vNames[a];
				if (!_stricmp(szTemp, vRef.c_str())) 
				{
					return true;
				}
			}
			if (CheckGroup(szName)) // Lets check if they are in our group
			{
				return true;
			}
			if (CheckEQBC(szName)) // Lets check if they are connected to EQBC server
			{
				return true;
			}
			if (CheckMQ2DanNet(szName)) // Lets check if they are connected to MQ2DanNet
			{
				return true;
			}
		}
	}
	return false;
}

void CheckvSeen(void) // We need to check if any of the vSeen characters have changed from friend to stranger or from stranger to friend
{
	PSPAWNINFO pNewSpawn;
	char szTemp[MAX_STRING];
	if (vSeen.size()) // Ok lets loop through vSeen and see if someone has joined our guild or fellowship
	{
		for (unsigned int a = 0; a < vSeen.size(); a++)
		{
			strcpy_s(szTemp, vSeen[a].c_str());
			pNewSpawn = (PSPAWNINFO)GetSpawnByName(szTemp);
			if (pNewSpawn && !_stricmp(pNewSpawn->DisplayedName, szTemp))
			{
				if (bIgnoreGuild)
				{
#if defined(NEWCHARINFO)
					if (GetCharInfo()->GuildID.GUID)
					{
						if (GetCharInfo()->GuildID.GUID == pNewSpawn->mPlayerPhysicsClient.pSpawn->GuildID) 
#else
					if (GetCharInfo()->GuildID)
					{
						if (GetCharInfo()->GuildID == pNewSpawn->mPlayerPhysicsClient.pSpawn->GuildID) 
#endif
						{
							vSeen.erase(vSeen.begin() + a);
							ClearFriendsAndStrangers(szTemp);
						}
					}
				}
				if (bIgnoreFellowship)
				{
					if (GetCharInfo()->pSpawn->Fellowship.FellowshipID)
					{
						FELLOWSHIPINFO Fellowship = (FELLOWSHIPINFO)GetCharInfo()->pSpawn->Fellowship;
						for (DWORD i = 0; i < Fellowship.Members; i++)
						{
							if (!_stricmp(Fellowship.FellowshipMember[i].Name, szTemp))
							{
								vSeen.erase(vSeen.begin() + a);
								ClearFriendsAndStrangers(szTemp);
							}
						}
					}
				}
			}
		}
	}
	if (vFriends.size())
	{
		for (unsigned int a = 0; a < vFriends.size(); a++)
		{
			strcpy_s(szTemp, vFriends[a].c_str());
			pNewSpawn = (PSPAWNINFO)GetSpawnByName(szTemp);
			if (pNewSpawn && !_stricmp(pNewSpawn->DisplayedName, szTemp))
			{
				if (!CheckNames(pNewSpawn->DisplayedName)) // Hey they are NOT our friend
				{
					vFriends.erase(vFriends.begin() + a); // Lets remove them from our friend list
					if (bNotify && bNotifyFriends)
					{
						WriteChatf("\ag%s has moved out of range.\ax", szTemp);
					}
					vStrangers.push_back(pNewSpawn->DisplayedName); // Lets add them to our stranger list
					if (bNotify && bNotifyStrangers)
					{
						WriteChatf("\ar%s is nearby.\ax", pNewSpawn->DisplayedName);
					}
					if (bAudio)
					{
						if (gAudioTimer <= MQGetTickCount64())
						{
							PlaySound(PosseSound, 0, SND_ASYNC);
							gAudioTimer = (unsigned int)MQGetTickCount64() + gAudioDelay;
						}
					}
					doCommands();
				}
			}
		}
	}
	if (vStrangers.size())
	{
		for (unsigned int a = 0; a < vStrangers.size(); a++)
		{
			strcpy_s(szTemp, vStrangers[a].c_str());
			pNewSpawn = (PSPAWNINFO)GetSpawnByName(szTemp);
			if (pNewSpawn && !_stricmp(pNewSpawn->DisplayedName, szTemp))
			{
				if (CheckNames(pNewSpawn->DisplayedName)) // Hey they ARE our friend
				{
					vStrangers.erase(vStrangers.begin() + a); // Lets remove them from our stranger list
					if (bNotify && bNotifyStrangers)
					{
						WriteChatf("\ar%s has moved out of range.\ax", szTemp);
					}
					if (bAudio)
					{
						if (gAudioTimer <= MQGetTickCount64())
						{
							PlaySound(PosseSound, 0, SND_ASYNC);
							gAudioTimer = (unsigned int)MQGetTickCount64() + gAudioDelay;
						}
					}
					vFriends.push_back(pNewSpawn->DisplayedName); // Lets add them to our friend list
					if (bNotify && bNotifyFriends)
					{
						WriteChatf("\ag%s is nearby.\ax", pNewSpawn->DisplayedName);
					}
				}
			}
		}
	}
}

void SpawnCheck(PSPAWNINFO pNewSpawn) 
{
	if (!bPosseEnabled)
	{
		return;
	}
	if (!pNewSpawn)
	{
		return;
	}
    if (pNewSpawn->GM == 1) 
	{
        WriteChatf("%s (\arGM\ax) is nearby.", pNewSpawn->DisplayedName);
        doCommands();
        return;
    }
    if (GetSpawnType(pNewSpawn) == PC) 
	{
        PSPAWNINFO pChar = GetCharInfo()->pSpawn;
		if (bIgnoreGuild)
		{
			if (bDebug) 
			{
				WriteChatf("My Name: %s Their Name: %s ", GetCharInfo()->Name, pNewSpawn->Name);
				WriteChatf("My Guild ID: %lu Their Guild ID: %lu  ", GetCharInfo()->GuildID, pNewSpawn->mPlayerPhysicsClient.pSpawn->GuildID);
			}
#if defined(NEWCHARINFO)
			if (GetCharInfo()->GuildID.GUID)
			{
				if (GetCharInfo()->GuildID.GUID == pNewSpawn->mPlayerPhysicsClient.pSpawn->GuildID)
#else
			if (GetCharInfo()->GuildID)
			{
				if (GetCharInfo()->GuildID == pNewSpawn->mPlayerPhysicsClient.pSpawn->GuildID)
#endif
				{
					return;
				}
			}
		}
		if (bIgnoreFellowship)
		{
			if (GetCharInfo()->pSpawn->Fellowship.FellowshipID)
			{
				FELLOWSHIPINFO Fellowship = (FELLOWSHIPINFO)pChar->Fellowship;
				for (DWORD i = 0; i < Fellowship.Members; i++)
				{
					if (!_stricmp(Fellowship.FellowshipMember[i].Name, pNewSpawn->Name))
					{
						return;
					}
				}
			}
		}
        if (pNewSpawn != pChar && DistanceToSpawn(pNewSpawn, pChar) < SpawnRadius)
		{
            for (register unsigned int a = 0; a < vSeen.size(); a++) // check if spawn is on the list already
			{
                string& vRef = vSeen[a];
                if (!_stricmp(pNewSpawn->DisplayedName,vRef.c_str())) // Yup they are already in vSeen
				{
					return;
                }
            }
            vSeen.push_back(pNewSpawn->DisplayedName); // if we get here this is a new spawn as add.
            if (CheckNames(pNewSpawn->DisplayedName)) // They are our friend
			{
				vFriends.push_back(pNewSpawn->DisplayedName); // Lets add them to our friend list
				if (bNotify && bNotifyFriends)
				{
					WriteChatf("\ag%s is nearby.\ax", pNewSpawn->DisplayedName);
				}
            } 
			else // They are a stranger
			{
                vStrangers.push_back(pNewSpawn->DisplayedName); // Lets add them to our stranger list
				if (bNotify && bNotifyStrangers)
				{
					WriteChatf("\ar%s is nearby.\ax", pNewSpawn->DisplayedName);
				}
                if (bAudio) 
				{
					if (gAudioTimer <= MQGetTickCount64())
					{
						PlaySound(PosseSound, 0, SND_ASYNC);
						gAudioTimer = (unsigned int)MQGetTickCount64() + gAudioDelay;
					}
                }
                doCommands();
             }
        }
    }
    return;
}

PLUGIN_API VOID OnZone(VOID) {
	vFriends.clear();
    vStrangers.clear();
    vSeen.clear ();
}

PLUGIN_API VOID InitializePlugin(VOID) {
    DebugSpewAlways("Initializing MQ2Posse");
    AddCommand("/posse", PosseCommand);
    pPosseType = new MQ2PosseType;
    AddMQ2Data("Posse",dataPosse);
}

PLUGIN_API VOID ShutdownPlugin(VOID) {
   DebugSpewAlways("Shutting down MQ2Posse");
   RemoveMQ2Data("Posse");
   delete pPosseType;
   RemoveCommand("/posse");
}

PLUGIN_API VOID SetGameState(DWORD GameState) {
    if(GameState==GAMESTATE_INGAME) {
        if (!bInitDone) LoadINI();
    } else if(GameState!=GAMESTATE_LOGGINGIN) {
        if (bInitDone) bInitDone=false;
    }
}

PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn) 
{
    if (vSeen.size()) 
	{
        for (unsigned int a = 0; a < vSeen.size(); a++) 
		{
            string& vRef = vSeen[a];
            if (!_stricmp(pSpawn->DisplayedName,vRef.c_str())) 
			{
				char delName[MAX_STRING];
                sprintf_s(delName,"%s",vRef.c_str());
                vSeen.erase(vSeen.begin() + a);
				ClearFriendsAndStrangers(delName);
            }
        }
    }
}

PLUGIN_API VOID OnPulse(VOID) 
{
	if (GetGameState() != GAMESTATE_INGAME || !bPosseEnabled)
	{
		return;
	}
	if (!bInitDone)
	{
		LoadINI();
	}
	SkipPulse++;
	if (SkipPulse == SKIP_PULSES)
	{
		SkipPulse = 0;
		char szTemp[MAX_STRING];
		sprintf_s(szTemp, "%s,", GetShortZone(GetCharInfo()->zoneId));
		if (strstr(szSkipZones, szTemp))
		{
			return;
		}
		PCHARINFO pCharInfo = GetCharInfo();
		PSPAWNINFO pNewSpawn;
		if (vSeen.size())
		{
			CheckvSeen(); // Lets see if anyone has moved from stranger to friend or friend to stranger
		}
		unsigned int sCount = CountMatchingSpawns(&mySpawn, pCharInfo->pSpawn, false);
		if (sCount) 
		{
			for (unsigned int a = 1; a <= sCount; a++) 
			{
				pNewSpawn = NthNearestSpawn(&mySpawn, a, pCharInfo->pSpawn, false);
				SpawnCheck(pNewSpawn);
			}
			if (!vSeen.size())
			{
				return;
			}
			bool bDidSee;
			for (unsigned int a = 0; a < vSeen.size(); a++) 
			{
				string& vRef = vSeen[a];
				sprintf_s(szTemp, "%s", vRef.c_str());
				bDidSee = false;
				for (unsigned int b = 1; b <= sCount; b++) 
				{
					pNewSpawn = NthNearestSpawn(&mySpawn, b, pCharInfo->pSpawn, false);
					if (!_stricmp(pNewSpawn->DisplayedName, szTemp))
					{
						bDidSee = true;
						break;
					}
				}
				if (!bDidSee) // Ok so they must have moved out of range
				{
					for (unsigned int b = 0; b < vSeen.size(); b++) 
					{
						string& vRef = vSeen[b];
						if (!_stricmp(vRef.c_str(), szTemp))
						{
							vSeen.erase(vSeen.begin() + b); // Lets remove them from vSeen
							ClearFriendsAndStrangers(szTemp); // Lets remove them from vFriends or vStrangers
							return;
						}
					}
				}
			}
		}
		else
		{ // Ok there is no one in range lets clear vSeen/vFriends/vStrangers
			if (vSeen.size()) 
			{
				for (unsigned int a = 0; a < vSeen.size(); a++) 
				{
					string& vRef = vSeen[a];
					sprintf_s(szTemp, "%s", vRef.c_str());
					vSeen.erase(vSeen.begin() + a);
					ClearFriendsAndStrangers(szTemp);
				}
			}
			else 
			{ // This shouldn't happen but if it does we need to clear vFriends/vStrangers
				vFriends.clear();
				vStrangers.clear();
			}
		}
	}
}
