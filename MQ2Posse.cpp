// MQ2Posse.cpp : RedGuides exclusive plugin to manage commands when friends and strangers are nearby
// v1.00 :: Sym - 2012-09-27
// v1.01 :: Sym - 2012-09-30
// v1.02 :: Sym - 2012-12-15
// v1.03 :: EqMule - 2015-03-09
// v1.04 :: Ctaylor22 - 2016-07-22

#include "../MQ2Plugin.h"
#include <vector>
#include "mmsystem.h"
#pragma comment(lib, "winmm.lib")

PLUGIN_VERSION(1.04);

//#pragma warning(disable:4244)
PreSetup("MQ2Posse");

#define SKIP_PULSES 50
vector <string> vSeen;
vector <string> vGlobalNames;
vector <string> vIniNames;
vector <string> vNames;
vector <string> vCommands;
SEARCHSPAWN mySpawn;

bool bPosseEnabled = false;
//bool bZoning = false;
bool bInitDone = false;
bool bIgnoreGuild = false;
bool bNotify = true;
bool bAudio = false;
bool bNotifyFriends = true;
bool bNotifyStrangers = true;
bool bWasEnabled = false;
bool bDebug = false;

unsigned int gAudioTimer=0;
unsigned int gAudioDelay=30000;
unsigned int gFriends = 0;
unsigned int gStrangers = 0;

long SkipPulse = 0;
int SpawnRadius = 300;
int SpawnZRadius = 30;
char szTemp[MAX_STRING];
char szName[MAX_STRING];
char szList[MAX_STRING];
char szSkipZones[MAX_STRING];
char PosseSound[MAX_STRING];
char delName[MAX_STRING];
char displayName[MAX_STRING];
char seenName[MAX_STRING];

class MQ2PosseType *pPosseType = 0;

class MQ2PosseType : public MQ2Type
{
public:
   enum PosseMembers {
      Status=1,
      Count=2,
	  Radius=3,
	  ZRadius=4,
	  Friends=5,
	  Strangers=6,
   };

   MQ2PosseType():MQ2Type("Posse")
   {
      TypeMember(Status);
      TypeMember(Count);
	  TypeMember(Radius);
	  TypeMember(ZRadius);
	  TypeMember(Friends);
	  TypeMember(Strangers);
   }

   ~MQ2PosseType()
   {
   }

   bool GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR &Dest)
   {
      PMQ2TYPEMEMBER pMember=MQ2PosseType::FindMember(Member);
      if (!pMember)
         return false;
      switch((PosseMembers)pMember->ID)
      {
         case Status:
			Dest.Int=bPosseEnabled;
            Dest.Type=pBoolType;
            return true;
         case Count:
			Dest.Int=vSeen.size();
            Dest.Type=pIntType;
            return true;
		 case Radius:
			Dest.Int=SpawnRadius;
            Dest.Type=pIntType;
			return true;
		 case ZRadius:
			Dest.Int=SpawnZRadius;
            Dest.Type=pIntType;
			return true;
         case Friends:
			Dest.Int=gFriends;
            Dest.Type=pIntType;
            return true;
         case Strangers:
			Dest.Int=gStrangers;
            Dest.Type=pIntType;
            return true;
      }
      return false;
   }

   bool ToString(MQ2VARPTR VarPtr, PCHAR Destination)
   {
      strcpy_s(Destination, MAX_STRING, "FALSE");
      if (bPosseEnabled)
         strcpy_s(Destination, MAX_STRING,"TRUE");
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

void ListUsers() {
    WriteChatf("User list contains \ay%d\ax %s", vNames.size(), vNames.size() > 1 ? "entries" : "entry");
    for (unsigned int a = 0; a < vNames.size(); a++) {
        string& vRef = vNames[a];
        WriteChatf("\at%d: %s\ax", a+1, vRef.c_str());
    }
}

void ListCommands() {
    WriteChatf("Command list contains \ay%d\ax %s", vCommands.size(), vCommands.size() > 1 ? "entries" : "entry");
    for (unsigned int a = 0; a < vCommands.size(); a++) {
        string& vRef = vCommands[a];
        WriteChatf("\at%d: %s\ax", a+1, vRef.c_str());
    }
}

void CombineNames() {
	vNames.clear();
	for (register unsigned int a = 0; a < vGlobalNames.size(); a++) {
		string& vRef = vGlobalNames[a];
		vNames.push_back(vRef.c_str()); 
	}
	for (register unsigned int a = 0; a < vIniNames.size(); a++) {
		string& vRef = vIniNames[a];
		vNames.push_back(vRef.c_str()); 
	}
}


void Update_INIFileName() {
    //sprintf_s(INIFileName,"%s\\%s_%s.ini",gszINIPath, EQADDR_SERVERNAME, GetCharInfo()->Name);
	sprintf_s(INIFileName,"%s\\MQ2Posse.ini",gszINIPath);
}

void ShowStatus(void) {
	WriteChatf("\atMQ2Posse :: v%1.2f :: by Sym for RedGuides.com\ax", MQ2Version);
	WriteChatf("MQ2Posse :: %s", bPosseEnabled?"\agENABLED\ax":"\arDISABLED\ax");
	WriteChatf("MQ2Posse :: Radius is \ag%d\ax", SpawnRadius);
	WriteChatf("MQ2Posse :: ZRadius is \ag%d\ax", SpawnZRadius);
	WriteChatf("MQ2Posse :: Ignore guildmates is %s", bIgnoreGuild ? "\agON\ax" : "\arOFF\ax");
	WriteChatf("MQ2Posse :: Notfications are %s", bNotify ? "\agON\ax" : "\arOFF\ax");
	WriteChatf("MQ2Posse :: Friend notfications are %s", bNotifyFriends ? "\agON\ax" : "\arOFF\ax");
	WriteChatf("MQ2Posse :: Stranger notfications are %s", bNotifyStrangers ? "\agON\ax" : "\arOFF\ax");
	WriteChatf("MQ2Posse :: Audio alerts are %s", bAudio ? "\agON\ax" : "\arOFF\ax");
	WriteChatf("MQ2Posse :: Zones to skip: \ag%s\ax",szSkipZones);
	CombineNames();
	if (vNames.size()) ListUsers();
	if (vCommands.size()) ListCommands();	
}




VOID LoadINI(VOID) {

    Update_INIFileName();
    // get on/off settings
    sprintf_s(szTemp,"%s_Settings",GetCharInfo()->Name);
	bPosseEnabled = GetPrivateProfileInt(szTemp, "Enabled", 0, INIFileName) > 0 ? true : false;
	SpawnRadius = GetPrivateProfileInt(szTemp, "Radius", 300, INIFileName);
	SpawnZRadius = GetPrivateProfileInt(szTemp, "ZRadius", 30, INIFileName);
	bNotify = GetPrivateProfileInt(szTemp, "Notify", 1, INIFileName) > 0 ? true : false;
	bNotifyFriends = GetPrivateProfileInt(szTemp, "NotifyFriends", 1, INIFileName) > 0 ? true : false;
	bNotifyStrangers = GetPrivateProfileInt(szTemp, "NotifyStrangers", 1, INIFileName) > 0 ? true : false;
	bIgnoreGuild = GetPrivateProfileInt(szTemp, "IgnoreGuild", 0, INIFileName) > 0 ? true : false;
	bAudio = GetPrivateProfileInt(szTemp, "Audio", 0, INIFileName) > 0 ? true : false;
	sprintf_s(szName,"%s\\mq2posse.wav",gszINIPath);
	GetPrivateProfileString(szTemp,"Soundfile",szName,PosseSound,2047,INIFileName);
	GetPrivateProfileString(szTemp,"SkipZones","poknowledge,guildhall,guildlobby,bazaar,neighborhood",szSkipZones,2047,INIFileName);

    // get all names
    sprintf_s(szTemp,"%s_Names",GetCharInfo()->Name);
    GetPrivateProfileSection(szTemp,szList,MAX_STRING,INIFileName);

    // clear list
    vIniNames.clear();

    char* p = (char*)szList;
    char *pch, *next_pch;
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
	gFriends = 0;
	gStrangers = 0;
	// flag first load init as done
	bInitDone = true;
}

VOID SaveINI(VOID) {
	char ToStr[16];

    Update_INIFileName();
    // write on/off settings
    sprintf_s(szTemp,"%s_Settings",GetCharInfo()->Name);
    WritePrivateProfileSection(szTemp, "", INIFileName);
	WritePrivateProfileString(szTemp,"Enabled",bPosseEnabled? "1" : "0",INIFileName);
	sprintf_s(ToStr,"%d",SpawnRadius);
	WritePrivateProfileString(szTemp,"Radius",ToStr,INIFileName);
	sprintf_s(ToStr,"%d",SpawnZRadius);
	WritePrivateProfileString(szTemp,"ZRadius",ToStr,INIFileName);
	WritePrivateProfileString(szTemp,"Soundfile",PosseSound,INIFileName);
	WritePrivateProfileString(szTemp,"Audio",bAudio? "1" : "0",INIFileName);
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
	WriteChatf("\ay${Posse.Strangers}\ax :: Returns how many strangers are in radius");
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

BOOL CheckNames(PCHAR szName) {
    CHAR szTemp[MAX_STRING];
    if (gGameState==GAMESTATE_INGAME) {
        if (pCharData) {
            strcpy_s(szTemp, szName);
            _strlwr_s(szTemp);
            for (register unsigned int a = 0; a < vNames.size(); a++) {
                string& vRef = vNames[a];
                if(!_stricmp(szTemp, vRef.c_str())) {
                    return true;
				}
            }
        }
    }
    return false;
}

void CheckGuild(void) {
    if (vSeen.size()) {
	    CHAR szTemp[MAX_STRING];
		PSPAWNINFO pNewSpawn;
		for (register unsigned int a = 0; a < vSeen.size(); a++) {
			strcpy_s(szTemp,vSeen[a].c_str());
			pNewSpawn = (PSPAWNINFO)GetSpawnByName(szTemp);
			if (pNewSpawn && !_stricmp(pNewSpawn->DisplayedName,szTemp)) {
				if (bIgnoreGuild && GetCharInfo()->GuildID == pNewSpawn->mPlayerPhysicsClient.pSpawn->GuildID) {
			        sprintf_s(delName,"%s",szTemp);
			        vSeen.erase(vSeen.begin() + a);	
					gStrangers--;
					if (bNotify && bNotifyStrangers) WriteChatf("\ar%s has been removed from list.\ax", delName);
					if (bAudio) if (gAudioTimer<=MQGetTickCount64()) {
						PlaySound(PosseSound,0,SND_ASYNC);
						gAudioTimer=(unsigned int)MQGetTickCount64()+gAudioDelay;
					}
				}
			}		
		}
	}
}

void SpawnCheck(PSPAWNINFO pNewSpawn) {
	if (!pNewSpawn) return;
    if (pNewSpawn->GM == 1) {
        WriteChatf("%s (\arGM\ax) is nearby.", pNewSpawn->DisplayedName);
        doCommands();
		return;
	}

    if (GetSpawnType(pNewSpawn) == PC) {
        PSPAWNINFO pChar = GetCharInfo()->pSpawn;
		if (bDebug) {
			PSPAWNINFO pChar1 = (PSPAWNINFO)GetSpawnByID(pNewSpawn->SpawnID);
			WriteChatf("My Name: %s : %s : %s ", GetCharInfo()->Name, pNewSpawn->Name, pChar1->Name);
			WriteChatf("My Guild ID: %lu ID: %lu ID: %lu ", GetCharInfo()->GuildID, pNewSpawn->mPlayerPhysicsClient.pSpawn->GuildID, pChar1->mPlayerPhysicsClient.pSpawn->GuildID);
		}
		if (bIgnoreGuild && GetCharInfo()->GuildID == pNewSpawn->mPlayerPhysicsClient.pSpawn->GuildID) return;
        if (pNewSpawn != pChar && DistanceToSpawn(pNewSpawn, pChar) < SpawnRadius) {
			// check if spawn is on the list already
			for (register unsigned int a = 0; a < vSeen.size(); a++) {
		        string& vRef = vSeen[a];
				if (!_stricmp(pNewSpawn->DisplayedName,vRef.c_str())) {
						return;
							}
			}
			// if we get here this is a new spawn as add.
			vSeen.push_back(pNewSpawn->DisplayedName);

            if (CheckNames(pNewSpawn->DisplayedName)) {
				gFriends++;
                if (bNotify && bNotifyFriends) WriteChatf("\ag%s is nearby.\ax", pNewSpawn->DisplayedName);
						} else {
							gStrangers++;
                if (bNotify && bNotifyStrangers) WriteChatf("\ar%s is nearby.\ax", pNewSpawn->DisplayedName);
				if (bAudio) if (gAudioTimer<=MQGetTickCount64()) {
					PlaySound(PosseSound,0,SND_ASYNC);
					gAudioTimer=(unsigned int)MQGetTickCount64()+gAudioDelay;
								}
							doCommands();
						}
					}
				}	
	return;
}


PLUGIN_API VOID OnZone(VOID) {
	//bZoning = true;
	gStrangers = 0;
	gFriends = 0;
}

/*
PLUGIN_API VOID OnEndZone(VOID) {
	sprintf_s(szName,"%s,",GetShortZone(GetCharInfo()->zoneId));
	sprintf_s(szTemp,"%s,",szSkipZones);
	if (strstr(szTemp,szName)) {
		bWasEnabled = bPosseEnabled;
		bPosseEnabled = false;
	} else {
		bPosseEnabled = bWasEnabled;
		bWasEnabled = false;
	}
	bZoning = false;
}
*/

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

PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn) {
	//for (register unsigned int a = 0; a < vSeen.size(); a++) {
    //    string& vRef = vSeen[a];
	//	if (!_stricmp(pSpawn->DisplayedName,vRef.c_str())) {
	//		vSeen.erase(vSeen.begin() + (a));		
	//		return;
	//	}
    //}
	if (vSeen.size()) {
		for (unsigned int a = 0; a < vSeen.size(); a++) {
			string& vRef = vSeen[a];
            if (!_stricmp(pSpawn->DisplayedName,vRef.c_str())) {
	            sprintf_s(delName,"%s",vRef.c_str());
			    vSeen.erase(vSeen.begin() + a);							
			    if (CheckNames(delName)) {
    				gFriends--;
	    			if (bNotify && bNotifyFriends) WriteChatf("\ag%s has moved out of range.\ax", delName);
		    	} else {
			    	gStrangers--;
    				if (bNotify && bNotifyStrangers) WriteChatf("\ar%s has moved out of range.\ax", delName);
	    			if (bAudio) if (gAudioTimer<=MQGetTickCount64()) {
		    			PlaySound(PosseSound,0,SND_ASYNC);
			    		gAudioTimer= (unsigned int)MQGetTickCount64()+gAudioDelay;
				    }
			    }
			}
		}
    }
}

PLUGIN_API VOID OnPulse(VOID) {
    if (GetGameState() != GAMESTATE_INGAME || !bPosseEnabled) return;
	if (!bInitDone) LoadINI();

	SkipPulse++;
    if (SkipPulse == SKIP_PULSES) {
        SkipPulse = 0;
		sprintf_s(szName,"%s,",GetShortZone(GetCharInfo()->zoneId));
		sprintf_s(szTemp,"%s,",szSkipZones);
		if (strstr(szTemp,szName))
			return;

		PCHARINFO pCharInfo=GetCharInfo();
		PSPAWNINFO pNewSpawn;
		if (vSeen.size()) 
			CheckGuild();
		unsigned int sCount = CountMatchingSpawns(&mySpawn, pCharInfo->pSpawn, false);
		if (sCount) {
			for (unsigned int s=1; s<=sCount; s++) {
				pNewSpawn = NthNearestSpawn(&mySpawn, s, pCharInfo->pSpawn, false);
				SpawnCheck(pNewSpawn);
			}
			if (!vSeen.size())
				return;
			bool bDidSee;
			for (unsigned int b = 0; b < vSeen.size(); b++) {				
				string& vRef = vSeen[b];
				sprintf_s(delName,"%s",vRef.c_str());
				bDidSee = false;
				for (unsigned int s=1; s<=sCount; s++) {
					pNewSpawn = NthNearestSpawn(&mySpawn, s, pCharInfo->pSpawn, false);					
					sprintf_s(displayName,"%s",pNewSpawn->DisplayedName);
					if (!_stricmp(displayName,delName)) {
							bDidSee = true;
							break;
						}
					}
				if (!bDidSee) {
					for (unsigned int c = 0; c < vSeen.size(); c++) {
						string& vRef = vSeen[c];						
						if (!_stricmp(vRef.c_str(),delName)) {
							vSeen.erase(vSeen.begin() + c);							
							if (CheckNames(delName)) {
								gFriends--;
								if (bNotify && bNotifyFriends) WriteChatf("\ag%s has moved out of range.\ax", delName);
							} else {
								gStrangers--;
								if (bNotify && bNotifyStrangers) WriteChatf("\ar%s has moved out of range.\ax", delName);
								if (bAudio) if (gAudioTimer<=MQGetTickCount64()) {
									PlaySound(PosseSound,0,SND_ASYNC);
									gAudioTimer= (unsigned int)MQGetTickCount64()+gAudioDelay;
								}
							}
							return;
						}						
					}
				}
			}
		} else {
			if (vSeen.size()) {
				for (unsigned int c = 0; c < vSeen.size(); c++) {
					string& vRef = vSeen[c];
					sprintf_s(delName,"%s",vRef.c_str());
					vSeen.erase(vSeen.begin() + c);							
					if (CheckNames(delName)) {
						gFriends--;
						if (bNotify && bNotifyFriends) WriteChatf("\ag%s has moved out of range.\ax", delName);
					} else {
						gStrangers--;
						if (bNotify && bNotifyStrangers) WriteChatf("\ar%s has moved out of range.\ax", delName);
						if (bAudio) if (gAudioTimer<=MQGetTickCount64()) {
							PlaySound(PosseSound,0,SND_ASYNC);
							gAudioTimer= (unsigned int)MQGetTickCount64()+gAudioDelay;
						}
					}
				}
			} else {
				if (gFriends) 
					gFriends = 0;
				if (gStrangers) 
					gStrangers = 0;
			}
		}
	}
}
