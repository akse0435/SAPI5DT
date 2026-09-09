/* DECtalk Voice Manager: design custom DECtalk voices and register them as SAPI5 voices. */
/* Parameter codes come from cmd/c_us_cde.h, limits from ph/ph_vdefi.c, defaults from ph/p_us_vdf_dectalk43.c. */
/* Builds as C with Visual Studio 6 and Visual Studio 2022. */

#define COBJMACROS

#ifndef _WIN32_WINNT
#	if defined(_MSC_VER) && _MSC_VER < 1300
#		define _WIN32_WINNT 0x0400
#	else
#		define _WIN32_WINNT 0x0601
#	endif
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <objbase.h>
#include <sapi.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Compatibility shims for the Visual Studio 6 headers                       */
/* ------------------------------------------------------------------------- */

#ifndef KEY_WOW64_64KEY
#	define KEY_WOW64_64KEY 0x0100
#endif
#ifndef KEY_WOW64_32KEY
#	define KEY_WOW64_32KEY 0x0200
#endif
#ifndef BS_MULTILINE
#	define BS_MULTILINE 0x00002000L
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#	define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#ifndef INVALID_FILE_SIZE
#	define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#endif

/* Defined locally so no sapi.lib is needed on either compiler. */
static const GUID DTVM_CLSID_SpVoice =
	{0x96749377,0x3391,0x11d2,{0x9e,0xe3,0x00,0xc0,0x4f,0x79,0x73,0x96}};
static const GUID DTVM_IID_ISpVoice =
	{0x6c44df74,0x72b9,0x4992,{0xa1,0xec,0xef,0x99,0x6e,0x04,0x22,0xd4}};
static const GUID DTVM_CLSID_SpObjectToken =
	{0xef411752,0x3736,0x4cb4,{0x9c,0x8c,0x8e,0xf4,0xcc,0xb5,0x8e,0xfe}};
static const GUID DTVM_IID_ISpObjectToken =
	{0x14056589,0xe16c,0x11d2,{0xbb,0x90,0x00,0xc0,0x4f,0x8e,0xe6,0xc0}};

/* ------------------------------------------------------------------------- */
/* Engine data                                                               */
/* ------------------------------------------------------------------------- */

#define NPARAM 28
#define NVOICE 9
#define MAXVOICES 256
#define MAXNAME 64

#define APPNAME     "DECtalk Voice Manager"
#define TOKENS_PATH "SOFTWARE\\Microsoft\\Speech\\Voices\\Tokens"
#define DT_CLSID    "{33954DF7-7F4C-4027-8022-3B3474490393}"
#define KEY_PREFIX  "DECtalk"
#define DISP_PREFIX "DECtalk "
#define DTV_MAGIC   "DECtalkVoice"

typedef struct
{
	const char *code;
	const char *label;
	const char *category;
	short       lo;
	short       hi;
} PARAMDEF;

static const PARAMDEF g_param[NPARAM] =
{
	{ "sx", "Sex (0 = female, 1 = male)",     "Voice",      0,    1 },
	{ "hs", "Head size (%)",                  "Voice",     65,  145 },
	{ "ap", "Average pitch (Hz)",             "Voice",     50,  350 },
	{ "pr", "Pitch range (%)",                "Voice",      0,  250 },
	{ "br", "Breathiness (dB)",               "Phonation",  0,   72 },
	{ "lx", "Lax breathiness (%)",            "Phonation",  0,  100 },
	{ "sm", "Smoothness (%)",                 "Phonation",  0,  100 },
	{ "ri", "Richness (%)",                   "Phonation",  0,  100 },
	{ "nf", "Fixed samples of open glottis",  "Phonation",  0,  100 },
	{ "la", "Laryngealization (%)",           "Phonation",  0,  100 },
	{ "f4", "Fourth formant frequency (Hz)",  "Formants" ,2000, 6000 },
	{ "b4", "Fourth formant bandwidth (Hz)",  "Formants" , 100, 6000 },
	{ "f5", "Fifth formant frequency (Hz)",   "Formants" ,2500, 6000 },
	{ "b5", "Fifth formant bandwidth (Hz)",   "Formants" , 100, 6000 },
	{ "gv", "Gain of voicing (dB)",           "Gains",      0,   87 },
	{ "gh", "Gain of aspiration (dB)",        "Gains",      0,   87 },
	{ "gf", "Gain of frication (dB)",         "Gains",      0,   87 },
	{ "gn", "Gain of nasalization (dB)",      "Gains",      0,   87 },
	{ "g1", "Gain of cascade formant 1 (dB)", "Gains",      0,   87 },
	{ "g2", "Gain of cascade formant 2 (dB)", "Gains",      0,   87 },
	{ "g3", "Gain of cascade formant 3 (dB)", "Gains",      0,   87 },
	{ "g4", "Gain of cascade formant 4 (dB)", "Gains",      0,   87 },
	{ "g5", "Loudness (dB)",                  "Gains",      0,   87 },
	{ "as", "Assertiveness (%)",              "Intonation", 0,  200 },
	{ "qu", "Quickness (%)",                  "Intonation", 0,  100 },
	{ "bf", "Baseline fall (Hz)",             "Intonation", 0,   90 },
	{ "hr", "Hat rise (Hz)",                  "Intonation", 2,  100 },
	{ "sr", "Stress rise (Hz)",               "Intonation", 1,  100 }
};

typedef struct
{
	const char *id;
	const char *label;
	const char *age;      /* the Age attribute installer.nsi gives this voice */
	char        select;   /* letter of the DECtalk [:nX] command */
} VOICEDEF;

static const VOICEDEF g_voice[NVOICE] =
{
	{ "paul",   "Perfect Paul",     "Adult",         'p' },
	{ "betty",  "Beautiful Betty",  "Adult",         'b' },
	{ "harry",  "Huge Harry",       "Adult",         'h' },
	{ "frank",  "Frail Frank",      "Senior; Adult", 'f' },
	{ "dennis", "Doctor Dennis",    "Adult",         'd' },
	{ "kit",    "Kit the Kid",      "Child",         'k' },
	{ "ursula", "Uppity Ursula",    "Senior; Adult", 'u' },
	{ "rita",   "Rough Rita",       "Adult",         'r' },
	{ "wendy",  "Whispering Wendy", "Adult",         'w' }
};

/* Same column order as g_param. */
static const short g_default[NVOICE][NPARAM] =
{
	{    1,  100,  122,  100,    0,    0,    3,   70,    0,    0, 3300,  260, 3650,  330,   65,   70,   70,   74,   68,   60,   48,   64,   86,  100,   40,   18,   18,   32 }, /* paul */
	{    0,  100,  208,  240,    0,   80,    4,   40,    0,    0, 4450,  260, 6000, 6000,   65,   70,   72,   72,   69,   65,   50,   56,   81,   35,   55,    0,   14,   20 }, /* betty */
	{    1,  115,   89,   80,    0,    0,   12,   86,   10,    0, 3300,  200, 3850,  240,   65,   70,   70,   73,   71,   60,   52,   62,   81,  100,   10,    9,   20,   30 }, /* harry */
	{    1,   90,  155,   90,   50,   50,   46,   40,    0,    5, 3650,  280, 4200,  300,   63,   68,   68,   75,   63,   58,   56,   66,   86,   65,    0,    9,   20,   22 }, /* frank */
	{    1,  105,  110,  135,   38,   70,  100,    0,   10,    0, 3200,  240, 3600,  280,   63,   68,   68,   76,   75,   60,   52,   61,   84,  100,   50,    9,   20,   22 }, /* dennis */
	{    0,   80,  306,  210,   47,   75,    5,   40,    0,    0, 6000, 6000, 6000, 6000,   65,   70,   72,   71,   69,   69,   52,   50,   73,   65,   50,    0,   20,   22 }, /* kit */
	{    0,   95,  240,  135,    0,   50,   60,  100,   10,    0, 4450,  260, 6000, 6000,   65,   70,   70,   74,   67,   65,   51,   58,   80,  100,   30,    8,   20,   32 }, /* ursula */
	{    0,   95,  106,   80,   46,    0,   24,   20,    0,    4, 4000,  250, 6000, 6000,   65,   70,   72,   73,   69,   72,   48,   54,   83,   65,   30,    0,   20,   32 }, /* rita */
	{    0,  100,  200,  175,   55,   80,  100,    0,   10,    0, 4500,  400, 6000, 6000,   51,   68,   70,   75,   69,   62,   53,   55,   83,   50,   10,    0,   20,   22 }  /* wendy */
};

typedef struct
{
	char name[MAXNAME];              /* display name as typed              */
	char key[MAXNAME + 16];          /* registry key name under Tokens     */
	int  base;                       /* index into g_voice                 */
	int  val[NPARAM];
} VOICEREC;

static VOICEREC g_store[MAXVOICES];
static int      g_count;

/* ------------------------------------------------------------------------- */
/* Small helpers                                                             */
/* ------------------------------------------------------------------------- */

static HINSTANCE g_inst;
static HFONT     g_font;
static HFONT     g_fontBold;
static BOOL      g_readOnly;

static void msg(HWND owner, UINT icon, const char *text)
{
	MessageBoxA(owner, text, APPNAME, MB_OK | icon);
}

static BOOL ask(HWND owner, const char *text)
{
	return MessageBoxA(owner, text, APPNAME, MB_YESNO | MB_ICONWARNING) == IDYES;
}

static int clampParam(int i, int v)
{
	if (v < g_param[i].lo) v = g_param[i].lo;
	if (v > g_param[i].hi) v = g_param[i].hi;
	return v;
}

static int findParam(const char *code)
{
	int i;
	for (i = 0; i < NPARAM; i++)
		if (lstrcmpiA(code, g_param[i].code) == 0) return i;
	return -1;
}

static void loadDefaults(int base, int *val)
{
	int i;
	for (i = 0; i < NPARAM; i++) val[i] = g_default[base][i];
}

static int findVoice(const char *id)
{
	int i;
	for (i = 0; i < NVOICE; i++)
		if (lstrcmpiA(id, g_voice[i].id) == 0) return i;
	return -1;
}

static void trim(char *s)
{
	int n;
	char *p = s;
	while (*p == ' ' || *p == '\t') p++;
	if (p != s) MoveMemory(s, p, lstrlenA(p) + 1);
	n = lstrlenA(s);
	while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' ||
					 s[n - 1] == '\r' || s[n - 1] == '\n'))
		s[--n] = 0;
}

/* Registry key names cannot contain a backslash; keep it conservative. */
static void makeKeyName(const char *name, char *out, int cch)
{
	int i, n = 0;
	lstrcpynA(out, KEY_PREFIX, cch);
	n = lstrlenA(out);
	for (i = 0; name[i] && n < cch - 1; i++)
	{
		char c = name[i];
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') || c == '-' || c == '_')
			out[n++] = c;
		else
			out[n++] = '_';
	}
	out[n] = 0;
}

/* One pair per command: the parser's argument arrays hold only NPARAM (10). */
static void buildDv(const int *val, char *out, int cch)
{
	char part[32];
	int  i;
	out[0] = 0;
	for (i = 0; i < NPARAM; i++)
	{
		wsprintfA(part, "[:dv %s %d]", g_param[i].code, val[i]);
		if (lstrlenA(out) + lstrlenA(part) + 1 >= cch) break;
		lstrcatA(out, part);
	}
}

/* Parse "[:dv code value ...]" back into val[], leaving untouched codes as-is. */
static void parseDv(const char *s, int *val)
{
	char        code[16];
	const char *p;
	int         i, k;

	p = s;
	while ((p = strstr(p, "dv")) != NULL)
	{
		p += 2;
		for (;;)
		{
			while (*p == ' ' || *p == '\t') p++;
			if (*p == 0 || *p == ']' || *p == '[' || *p == ':') break;
			i = 0;
			while (*p && *p != ' ' && *p != ']' && i < 15) code[i++] = *p++;
			code[i] = 0;
			while (*p == ' ' || *p == '\t') p++;
			if (*p == 0 || *p == ']') break;
			k = findParam(code);
			if (k >= 0) val[k] = clampParam(k, atoi(p));
			while (*p && *p != ' ' && *p != ']') p++;
		}
	}
}

/* ------------------------------------------------------------------------- */
/* Registry access                                                           */
/* ------------------------------------------------------------------------- */

static const REGSAM g_views[2] = { KEY_WOW64_32KEY, KEY_WOW64_64KEY };

static BOOL regGetStr(HKEY k, const char *name, char *buf, DWORD cb)
{
	DWORD type = 0;
	DWORD size = cb;
	ZeroMemory(buf, cb);
	if (RegQueryValueExA(k, name, NULL, &type, (LPBYTE)buf, &size) != ERROR_SUCCESS)
		return FALSE;
	buf[cb - 1] = 0;
	return type == REG_SZ || type == REG_EXPAND_SZ;
}

static BOOL regGetDword(HKEY k, const char *name, DWORD *out)
{
	DWORD type = 0;
	DWORD size = sizeof(DWORD);
	if (RegQueryValueExA(k, name, NULL, &type, (LPBYTE)out, &size) != ERROR_SUCCESS)
		return FALSE;
	return type == REG_DWORD;
}

static void regSetStr(HKEY k, const char *name, const char *value)
{
	RegSetValueExA(k, name, 0, REG_SZ, (const BYTE *)value, lstrlenA(value) + 1);
}

static void regSetDword(HKEY k, const char *name, DWORD value)
{
	RegSetValueExA(k, name, 0, REG_DWORD, (const BYTE *)&value, sizeof(value));
}

/* RegDeleteKey inherits the registry view from the parent handle. */
static void regDeleteTree(HKEY parent, const char *sub)
{
	HKEY  h;
	char  name[256];
	DWORD n;

	if (RegOpenKeyExA(parent, sub, 0, KEY_READ | KEY_WRITE, &h) == ERROR_SUCCESS)
	{
		for (;;)
		{
			n = sizeof(name);
			if (RegEnumKeyExA(h, 0, name, &n, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
				break;
			regDeleteTree(h, name);
		}
		RegCloseKey(h);
	}
	RegDeleteKeyA(parent, sub);
}

/* Which registry views hold an installed DECtalk engine. */
static BOOL g_viewOk[2];
static int  g_viewCount;

#ifdef _WIN64
#	define OWN_VIEW 1
#else
#	define OWN_VIEW 0
#endif

static BOOL is64BitWindows(void)
{
#ifdef _WIN64
	return TRUE;
#else
	typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
	SYSTEM_INFO si;
	PGNSI fn = (PGNSI)GetProcAddress(GetModuleHandleA("kernel32"),
									 "GetNativeSystemInfo");
	if (fn == NULL) return FALSE;
	ZeroMemory(&si, sizeof(si));
	fn(&si);
	return si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL;
#endif
}

/* InprocServer32 must name a DLL that is really on disk, unlike a leftover token. */
static BOOL engineInView(REGSAM view)
{
	HKEY h;
	char path[MAX_PATH];
	BOOL ok = FALSE;

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
					  "SOFTWARE\\Classes\\CLSID\\" DT_CLSID "\\InprocServer32",
					  0, KEY_READ | view, &h) == ERROR_SUCCESS)
	{
		if (regGetStr(h, NULL, path, sizeof(path)) && path[0] != 0)
			ok = GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
		RegCloseKey(h);
	}
	return ok;
}

static void detectViews(void)
{
	int i;

	g_viewCount = 0;
	for (i = 0; i < 2; i++)
	{
		g_viewOk[i] = engineInView(g_views[i]);
		if (g_viewOk[i]) g_viewCount++;
	}
	/* On 32-bit Windows both flags are ignored, so the two probes hit one key. */
	if (g_viewCount == 2 && !is64BitWindows())
	{
		g_viewOk[1] = FALSE;
		g_viewCount = 1;
	}
}

static void describeViews(char *out, int cch)
{
	if (g_viewOk[0] && g_viewOk[1])
		lstrcpynA(out, "DECtalk found: 32-bit and 64-bit", cch);
	else if (g_viewOk[0])
		lstrcpynA(out, "DECtalk found: 32-bit only", cch);
	else if (g_viewOk[1])
		lstrcpynA(out, "DECtalk found: 64-bit only", cch);
	else
		lstrcpynA(out, "No registered DECtalk engine found", cch);
}

/* Test the views we will actually write to, not the one this process runs in. */
static BOOL canWriteTokens(void)
{
	HKEY h;
	int  i;

	for (i = 0; i < 2; i++)
	{
		if (!g_viewOk[i]) continue;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TOKENS_PATH, 0,
						  KEY_WRITE | g_views[i], &h) != ERROR_SUCCESS)
			return FALSE;
		RegCloseKey(h);
	}
	return TRUE;
}

static BOOL writeVoiceView(const VOICEREC *v, REGSAM view)
{
	HKEY tokens = NULL, token = NULL, attrs = NULL;
	char dv[768];
	char disp[MAXNAME + 16];

	if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, TOKENS_PATH, 0, NULL, 0,
						KEY_READ | KEY_WRITE | view, NULL, &tokens, NULL) != ERROR_SUCCESS)
		return FALSE;
	if (RegCreateKeyExA(tokens, v->key, 0, NULL, 0,
						KEY_READ | KEY_WRITE, NULL, &token, NULL) != ERROR_SUCCESS)
	{
		RegCloseKey(tokens);
		return FALSE;
	}

	buildDv(v->val, dv, sizeof(dv));

	lstrcpynA(disp, DISP_PREFIX, sizeof(disp));
	lstrcatA(disp, v->name);
	regSetStr(token, NULL, disp);
	regSetStr(token, "409", disp);
	regSetStr(token, "CLSID", DT_CLSID);
	regSetDword(token, "Voice", (DWORD)v->base);
	regSetStr(token, "VoiceParams", dv);
	/* ap and pr double as the nominal pitch and range the engine scales against. */
	regSetDword(token, "VoicePitch", (DWORD)v->val[2]);
	regSetDword(token, "VoiceRange", (DWORD)v->val[3]);

	if (RegCreateKeyExA(token, "Attributes", 0, NULL, 0,
						KEY_READ | KEY_WRITE, NULL, &attrs, NULL) == ERROR_SUCCESS)
	{
		regSetStr(attrs, "Gender", v->val[0] ? "Male" : "Female");
		regSetStr(attrs, "Language", "409");
		regSetStr(attrs, "Age", g_voice[v->base].age);
		regSetStr(attrs, "Vendor", "DECtalk");
		regSetStr(attrs, "Name", v->name);
		RegCloseKey(attrs);
	}

	RegCloseKey(token);
	RegCloseKey(tokens);
	return TRUE;
}

/* Written to both views so 32-bit and 64-bit SAPI clients both see the voice. */
static BOOL storeSave(const VOICEREC *v)
{
	BOOL any = FALSE;
	int  i;
	for (i = 0; i < 2; i++)
		if (g_viewOk[i] && writeVoiceView(v, g_views[i])) any = TRUE;
	return any;
}

static void storeDelete(const char *key)
{
	HKEY tokens;
	int  i;
	for (i = 0; i < 2; i++)
	{
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TOKENS_PATH, 0,
						  KEY_READ | KEY_WRITE | g_views[i], &tokens) == ERROR_SUCCESS)
		{
			regDeleteTree(tokens, key);
			RegCloseKey(tokens);
		}
	}
}

/* TRUE when that key already holds a voice that is not one of ours. */
static BOOL keyTaken(const char *key)
{
	HKEY tokens, token;
	char buf[768];
	BOOL taken = FALSE;
	int  i;

	for (i = 0; i < 2; i++)
	{
		if (!g_viewOk[i]) continue;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TOKENS_PATH, 0,
						  KEY_READ | g_views[i], &tokens) != ERROR_SUCCESS) continue;
		if (RegOpenKeyExA(tokens, key, 0, KEY_READ, &token) == ERROR_SUCCESS)
		{
			if (!regGetStr(token, "VoiceParams", buf, sizeof(buf)) || buf[0] == 0)
				taken = TRUE;
			RegCloseKey(token);
		}
		RegCloseKey(tokens);
	}
	return taken;
}

static BOOL saveVoice(HWND owner, const VOICEREC *v)
{
	if (keyTaken(v->key))
	{
		msg(owner, MB_ICONERROR,
			"Another voice is already registered under that name. Pick a different name.");
		return FALSE;
	}
	if (g_viewCount == 0)
	{
		msg(owner, MB_ICONERROR,
			"No registered DECtalk SAPI5 engine was found, so there is nowhere to "
			"register the voice. Install DECtalk first.");
		return FALSE;
	}
	if (!storeSave(v))
	{
		msg(owner, MB_ICONERROR,
			"The voice could not be written to the registry. Run this program as "
			"administrator.");
		return FALSE;
	}
	return TRUE;
}

static int compareRecords(const void *a, const void *b)
{
	return lstrcmpiA(((const VOICEREC *)a)->name, ((const VOICEREC *)b)->name);
}

static void storeLoad(void)
{
	HKEY  tokens = NULL, token, attrs;
	char  key[256];
	char  buf[768];
	DWORD n, dw, index;
	int   i;

	g_count = 0;
	for (i = 0; i < 2; i++)
	{
		if (g_viewCount > 0 && !g_viewOk[i]) continue;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TOKENS_PATH, 0,
						  KEY_READ | g_views[i], &tokens) == ERROR_SUCCESS)
			break;
		tokens = NULL;
	}
	if (tokens == NULL) return;

	for (index = 0; g_count < MAXVOICES; index++)
	{
		n = sizeof(key);
		if (RegEnumKeyExA(tokens, index, key, &n, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			break;
		if (RegOpenKeyExA(tokens, key, 0, KEY_READ, &token) != ERROR_SUCCESS)
			continue;

		/* Ours are the DECtalk tokens carrying a VoiceParams value. */
		if (regGetStr(token, "CLSID", buf, sizeof(buf)) &&
			lstrcmpiA(buf, DT_CLSID) == 0 &&
			regGetStr(token, "VoiceParams", buf, sizeof(buf)) && buf[0] != 0)
		{
			VOICEREC *r = &g_store[g_count];
			ZeroMemory(r, sizeof(*r));
			lstrcpynA(r->key, key, sizeof(r->key));

			/* The plain name is in Attributes\\Name; the token values carry the prefix. */
			if (RegOpenKeyExA(token, "Attributes", 0, KEY_READ, &attrs) == ERROR_SUCCESS)
			{
				regGetStr(attrs, "Name", r->name, sizeof(r->name));
				RegCloseKey(attrs);
			}
			if (r->name[0] == 0)
				lstrcpynA(r->name, key + lstrlenA(KEY_PREFIX), sizeof(r->name));

			r->base = 0;
			if (regGetDword(token, "Voice", &dw) && (int)dw < NVOICE)
				r->base = (int)dw;

			loadDefaults(r->base, r->val);
			if (regGetStr(token, "VoiceParams", buf, sizeof(buf)))
				parseDv(buf, r->val);

			g_count++;
		}
		RegCloseKey(token);
	}
	RegCloseKey(tokens);

	if (g_count > 1)
		qsort(g_store, g_count, sizeof(VOICEREC), compareRecords);
}

static VOICEREC *storeFind(const char *name)
{
	int i;
	for (i = 0; i < g_count; i++)
		if (lstrcmpiA(g_store[i].name, name) == 0) return &g_store[i];
	return NULL;
}

/* ------------------------------------------------------------------------- */
/* .dtv export / import                                                      */
/* ------------------------------------------------------------------------- */

static BOOL exportDtv(const VOICEREC *v, const char *path)
{
	HANDLE f;
	char   line[256];
	DWORD  written;
	int    i;

	f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) return FALSE;

	wsprintfA(line, "%s=1\r\nName=%s\r\nBase=%s\r\n",
			  DTV_MAGIC, v->name, g_voice[v->base].id);
	WriteFile(f, line, lstrlenA(line), &written, NULL);
	for (i = 0; i < NPARAM; i++)
	{
		wsprintfA(line, "%s=%d\r\n", g_param[i].code, v->val[i]);
		WriteFile(f, line, lstrlenA(line), &written, NULL);
	}
	CloseHandle(f);
	return TRUE;
}

static BOOL importDtv(const char *path, VOICEREC *out, char *err, int errcch)
{
	HANDLE f;
	DWORD  size, read;
	char  *text;
	char  *line;
	char  *next;
	BOOL   magic = FALSE;

	f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE)
	{
		lstrcpynA(err, "The file could not be opened.", errcch);
		return FALSE;
	}
	size = GetFileSize(f, NULL);
	if (size == INVALID_FILE_SIZE || size > 65536)
	{
		CloseHandle(f);
		lstrcpynA(err, "That does not look like a DECtalk voice file.", errcch);
		return FALSE;
	}
	text = (char *)LocalAlloc(LPTR, size + 1);
	if (text == NULL)
	{
		CloseHandle(f);
		lstrcpynA(err, "Out of memory.", errcch);
		return FALSE;
	}
	ReadFile(f, text, size, &read, NULL);
	text[read] = 0;
	CloseHandle(f);

	ZeroMemory(out, sizeof(*out));
	out->base = 0;
	loadDefaults(0, out->val);

	line = text;
	while (line && *line)
	{
		char *eq;
		next = strpbrk(line, "\r\n");
		if (next) { *next = 0; next++; while (*next == '\r' || *next == '\n') next++; }

		trim(line);
		if (line[0] == 0 || line[0] == ';' || line[0] == '#') { line = next; continue; }

		eq = strchr(line, '=');
		if (eq == NULL) { line = next; continue; }
		*eq = 0;
		trim(line);
		eq++;
		trim(eq);

		if (lstrcmpiA(line, DTV_MAGIC) == 0)
			magic = TRUE;
		else if (lstrcmpiA(line, "Name") == 0)
			lstrcpynA(out->name, eq, sizeof(out->name));
		else if (lstrcmpiA(line, "Base") == 0)
		{
			int v = findVoice(eq);
			if (v < 0)
			{
				LocalFree(text);
				lstrcpynA(err, "The file names a base voice this engine does not have.", errcch);
				return FALSE;
			}
			out->base = v;
			loadDefaults(v, out->val);
		}
		else
		{
			int k = findParam(line);
			if (k >= 0) out->val[k] = clampParam(k, atoi(eq));
		}
		line = next;
	}
	LocalFree(text);

	if (!magic)
	{
		lstrcpynA(err, "That does not look like a DECtalk voice file.", errcch);
		return FALSE;
	}
	if (out->name[0] == 0)
	{
		lstrcpynA(err, "The file does not name the voice.", errcch);
		return FALSE;
	}
	makeKeyName(out->name, out->key, sizeof(out->key));
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Preview through SAPI5                                                     */
/* ------------------------------------------------------------------------- */

static ISpVoice *g_spVoice;
static BOOL      g_comReady;

/* First registered token whose CLSID is the DECtalk engine. */
static BOOL findDECtalkToken(char *key, int cch)
{
	HKEY  tokens = NULL, token;
	char  name[256];
	char  clsid[64];
	char  probe[64];
	DWORD n, index;
	BOOL  found = FALSE;

	/* SAPI resolves token ids through this process's own registry view. */
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TOKENS_PATH, 0,
					  KEY_READ | g_views[OWN_VIEW], &tokens) != ERROR_SUCCESS)
		return FALSE;

	for (index = 0; !found; index++)
	{
		n = sizeof(name);
		if (RegEnumKeyExA(tokens, index, name, &n, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			break;
		if (RegOpenKeyExA(tokens, name, 0, KEY_READ, &token) != ERROR_SUCCESS)
			continue;
		/* A built-in voice is the cleaner host, so skip our own tokens. */
		if (regGetStr(token, "CLSID", clsid, sizeof(clsid)) &&
			lstrcmpiA(clsid, DT_CLSID) == 0 &&
			!regGetStr(token, "VoiceParams", probe, sizeof(probe)))
		{
			lstrcpynA(key, name, cch);
			found = TRUE;
		}
		RegCloseKey(token);
	}
	RegCloseKey(tokens);
	return found;
}

static BOOL previewVoice(HWND owner, int base, const int *val)
{
	char             key[MAXNAME + 16];
	char             text[1024];
	char             dv[768];
	WCHAR            wide[1024];
	WCHAR            wid[600];
	char             id[600];
	ISpObjectToken  *token = NULL;
	HRESULT          hr;

	if (!g_comReady)
	{
		msg(owner, MB_ICONERROR, "COM could not be initialised, so no preview is possible.");
		return FALSE;
	}
	if (g_spVoice == NULL)
	{
		hr = CoCreateInstance(&DTVM_CLSID_SpVoice, NULL, CLSCTX_ALL,
							  &DTVM_IID_ISpVoice, (void **)&g_spVoice);
		if (FAILED(hr) || g_spVoice == NULL)
		{
			msg(owner, MB_ICONERROR, "SAPI5 could not be started, so no preview is possible.");
			g_spVoice = NULL;
			return FALSE;
		}
	}
	if (!g_viewOk[OWN_VIEW] || !findDECtalkToken(key, sizeof(key)))
	{
		msg(owner, MB_ICONINFORMATION,
			OWN_VIEW
				? "The 64-bit DECtalk engine (ttseng64.dll) is not installed, so this "
				  "build of the voice manager cannot play a preview. Designing and "
				  "saving voices still works."
				: "The 32-bit DECtalk engine (ttseng.dll) is not installed, so this "
				  "build of the voice manager cannot play a preview. Designing and "
				  "saving voices still works.");
		return FALSE;
	}

	lstrcpynA(id, "HKEY_LOCAL_MACHINE\\" TOKENS_PATH "\\", sizeof(id));
	lstrcatA(id, key);
	MultiByteToWideChar(CP_ACP, 0, id, -1, wid, sizeof(wid) / sizeof(WCHAR));

	hr = CoCreateInstance(&DTVM_CLSID_SpObjectToken, NULL, CLSCTX_ALL,
						  &DTVM_IID_ISpObjectToken, (void **)&token);
	if (SUCCEEDED(hr) && token != NULL)
	{
		if (SUCCEEDED(ISpObjectToken_SetId(token, NULL, wid, FALSE)))
			ISpVoice_SetVoice(g_spVoice, token);
		ISpObjectToken_Release(token);
	}

	/* The base voice must come first because selecting it loads its definition. */
	buildDv(val, dv, sizeof(dv));
	wsprintfA(text, "[:n%c]%s This is a preview of the DECtalk voice you are designing.",
			  g_voice[base].select, dv);
	MultiByteToWideChar(CP_ACP, 0, text, -1, wide, sizeof(wide) / sizeof(WCHAR));

	ISpVoice_Speak(g_spVoice, wide, SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
	return TRUE;
}

static void previewStop(void)
{
	if (g_spVoice != NULL)
	{
		ISpVoice_Speak(g_spVoice, L"", SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
		ISpVoice_Release(g_spVoice);
		g_spVoice = NULL;
	}
}

/* ------------------------------------------------------------------------- */
/* Editor window                                                             */
/* ------------------------------------------------------------------------- */

#define IDC_NAME    1001
#define IDC_BASE    1002
#define IDC_PANEL   1003
#define IDC_TEST    1004
#define IDC_EDIT0   2000
#define IDC_SPIN0   3000

#define ROWH        24
#define CAPH        22
#define LABELW      280
#define EDITW       76
#define PANELW      430
#define PANELH      300

typedef struct
{
	HWND     dlg;
	HWND     name;
	HWND     base;
	HWND     panel;
	HWND     edit[NPARAM];
	HWND     spin[NPARAM];
	int      panelPos;
	int      panelHeight;
	BOOL     isNew;
	BOOL     done;
	BOOL     saved;
	char     original[MAXNAME];
	VOICEREC rec;
} EDITOR;

static EDITOR g_ed;

static void panelUpdateScrollbar(void)
{
	SCROLLINFO si;
	RECT       rc;

	GetClientRect(g_ed.panel, &rc);
	ZeroMemory(&si, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin   = 0;
	si.nMax   = g_ed.panelHeight - 1;
	si.nPage  = rc.bottom;
	si.nPos   = g_ed.panelPos;
	SetScrollInfo(g_ed.panel, SB_VERT, &si, TRUE);
}

static void panelScrollTo(int pos)
{
	RECT rc;
	int  maxPos, delta;

	GetClientRect(g_ed.panel, &rc);
	maxPos = g_ed.panelHeight - rc.bottom;
	if (maxPos < 0) maxPos = 0;
	if (pos < 0) pos = 0;
	if (pos > maxPos) pos = maxPos;
	delta = pos - g_ed.panelPos;
	if (delta == 0) return;
	g_ed.panelPos = pos;
	ScrollWindowEx(g_ed.panel, 0, -delta, NULL, NULL, NULL, NULL,
				   SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
	panelUpdateScrollbar();
}

/* Tabbing can reach controls that are scrolled out of sight. */
static void panelScrollIntoView(HWND child)
{
	RECT rc, client;

	GetWindowRect(child, &rc);
	MapWindowPoints(NULL, g_ed.panel, (POINT *)&rc, 2);
	GetClientRect(g_ed.panel, &client);

	if (rc.top < 0)
		panelScrollTo(g_ed.panelPos + rc.top - 4);
	else if (rc.bottom > client.bottom)
		panelScrollTo(g_ed.panelPos + (rc.bottom - client.bottom) + 4);
}

static LRESULT CALLBACK panelProc(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp)
{
	switch (umsg)
	{
	case WM_VSCROLL:
		{
			SCROLLINFO si;
			int        pos = g_ed.panelPos;
			ZeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask  = SIF_ALL;
			GetScrollInfo(hwnd, SB_VERT, &si);
			switch (LOWORD(wp))
			{
			case SB_LINEUP:       pos -= ROWH; break;
			case SB_LINEDOWN:     pos += ROWH; break;
			case SB_PAGEUP:       pos -= si.nPage; break;
			case SB_PAGEDOWN:     pos += si.nPage; break;
			case SB_TOP:          pos = 0; break;
			case SB_BOTTOM:       pos = g_ed.panelHeight; break;
			case SB_THUMBTRACK:
			case SB_THUMBPOSITION: pos = si.nTrackPos; break;
			default: return 0;
			}
			panelScrollTo(pos);
		}
		return 0;

	case WM_MOUSEWHEEL:
		panelScrollTo(g_ed.panelPos - (((short)HIWORD(wp)) / WHEEL_DELTA) * ROWH * 3);
		return 0;

	case WM_COMMAND:
		if (HIWORD(wp) == EN_SETFOCUS)
			panelScrollIntoView((HWND)lp);
		else if (HIWORD(wp) == EN_CHANGE || HIWORD(wp) == EN_KILLFOCUS)
			return SendMessageA(GetParent(hwnd), umsg, wp, lp);
		return 0;

	case WM_NOTIFY:
		return SendMessageA(GetParent(hwnd), umsg, wp, lp);
	}
	return DefWindowProcA(hwnd, umsg, wp, lp);
}

static HWND mkChild(HWND parent, const char *cls, const char *text,
					DWORD style, int x, int y, int w, int h, int id)
{
	HWND c = CreateWindowExA(0, cls, text, WS_CHILD | WS_VISIBLE | style,
							 x, y, w, h, parent, (HMENU)(INT_PTR)id, g_inst, NULL);
	if (c) SendMessageA(c, WM_SETFONT, (WPARAM)g_font, TRUE);
	return c;
}

static void editorReadValues(void)
{
	char buf[32];
	int  i;
	for (i = 0; i < NPARAM; i++)
	{
		GetWindowTextA(g_ed.edit[i], buf, sizeof(buf));
		g_ed.rec.val[i] = clampParam(i, atoi(buf));
	}
	g_ed.rec.base = (int)SendMessageA(g_ed.base, CB_GETCURSEL, 0, 0);
	if (g_ed.rec.base < 0 || g_ed.rec.base >= NVOICE) g_ed.rec.base = 0;
	GetWindowTextA(g_ed.name, g_ed.rec.name, sizeof(g_ed.rec.name));
	trim(g_ed.rec.name);
}

static void editorShowValues(const int *val)
{
	char buf[32];
	int  i;
	for (i = 0; i < NPARAM; i++)
	{
		wsprintfA(buf, "%d", val[i]);
		SetWindowTextA(g_ed.edit[i], buf);
		if (g_ed.spin[i] != NULL)
			SendMessageA(g_ed.spin[i], UDM_SETPOS, 0, MAKELONG((short)val[i], 0));
	}
}

static void editorBuild(HWND hwnd)
{
	int  y = 12, i;
	const char *lastCat = NULL;

	mkChild(hwnd, "STATIC", "&Name:", SS_LEFT, 12, y + 4, 76, 18, -1);
	g_ed.name = mkChild(hwnd, "EDIT", g_ed.rec.name,
						WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 92, y, 170, 22, IDC_NAME);
	SendMessageA(g_ed.name, EM_SETLIMITTEXT, MAXNAME - 1, 0);

	mkChild(hwnd, "STATIC", "&Base voice:", SS_LEFT, 12, y + 32, 76, 18, -1);
	g_ed.base = mkChild(hwnd, "COMBOBOX", NULL,
						WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
						92, y + 28, 220, 200, IDC_BASE);
	for (i = 0; i < NVOICE; i++)
		SendMessageA(g_ed.base, CB_ADDSTRING, 0, (LPARAM)g_voice[i].label);
	SendMessageA(g_ed.base, CB_SETCURSEL, g_ed.rec.base, 0);

	y += 64;
	g_ed.panel = CreateWindowExA(WS_EX_CONTROLPARENT | WS_EX_CLIENTEDGE,
								 "DTVMPanel", NULL,
								 WS_CHILD | WS_VISIBLE | WS_VSCROLL,
								 12, y, PANELW, PANELH, hwnd,
								 (HMENU)(INT_PTR)IDC_PANEL, g_inst, NULL);

	{
		int py = 6;
		char label[160];
		for (i = 0; i < NPARAM; i++)
		{
			if (lastCat == NULL || lstrcmpA(lastCat, g_param[i].category) != 0)
			{
				HWND cap = mkChild(g_ed.panel, "STATIC", g_param[i].category,
								   SS_LEFT, 8, py + 4, LABELW, 18, -1);
				if (cap) SendMessageA(cap, WM_SETFONT, (WPARAM)g_fontBold, TRUE);
				lastCat = g_param[i].category;
				py += CAPH;
			}
			wsprintfA(label, "%s (%d to %d):", g_param[i].label,
					  (int)g_param[i].lo, (int)g_param[i].hi);
			mkChild(g_ed.panel, "STATIC", label, SS_LEFT, 20, py + 4, LABELW, 18, -1);
			g_ed.edit[i] = mkChild(g_ed.panel, "EDIT", NULL,
								   WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL,
								   20 + LABELW + 8, py, EDITW, 22, IDC_EDIT0 + i);
			g_ed.spin[i] = CreateWindowExA(0, UPDOWN_CLASSA, NULL,
										   WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT |
										   UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
										   0, 0, 0, 0, g_ed.panel,
										   (HMENU)(INT_PTR)(IDC_SPIN0 + i), g_inst, NULL);
			if (g_ed.spin[i])
			{
				SendMessageA(g_ed.spin[i], UDM_SETBUDDY, (WPARAM)g_ed.edit[i], 0);
				SendMessageA(g_ed.spin[i], UDM_SETRANGE, 0,
							 MAKELONG(g_param[i].hi, g_param[i].lo));
			}
			py += ROWH;
		}
		g_ed.panelHeight = py + 6;
	}
	editorShowValues(g_ed.rec.val);
	g_ed.panelPos = 0;
	panelUpdateScrollbar();

	y += PANELH + 12;
	g_ed.dlg = hwnd;
	mkChild(hwnd, "BUTTON", "&Test", WS_TABSTOP | BS_PUSHBUTTON, 12, y, 90, 26, IDC_TEST);
	mkChild(hwnd, "BUTTON", "OK", WS_TABSTOP | BS_DEFPUSHBUTTON,
			PANELW + 12 - 190, y, 90, 26, IDOK);
	mkChild(hwnd, "BUTTON", "Cancel", WS_TABSTOP | BS_PUSHBUTTON,
			PANELW + 12 - 92, y, 90, 26, IDCANCEL);
}

static void editorOk(HWND hwnd)
{
	VOICEREC *clash;

	editorReadValues();
	if (g_ed.rec.name[0] == 0)
	{
		msg(hwnd, MB_ICONERROR, "The voice needs a name.");
		SetFocus(g_ed.name);
		return;
	}
	clash = storeFind(g_ed.rec.name);
	if (clash != NULL && lstrcmpiA(g_ed.rec.name, g_ed.original) != 0)
	{
		char text[256];
		wsprintfA(text, "A custom voice named %s already exists. Overwrite it?",
				  g_ed.rec.name);
		if (!ask(hwnd, text)) return;
	}
	makeKeyName(g_ed.rec.name, g_ed.rec.key, sizeof(g_ed.rec.key));

	if (!saveVoice(hwnd, &g_ed.rec)) return;
	/* Renaming: drop the old token after the new one is written. */
	if (!g_ed.isNew && g_ed.original[0] != 0 &&
		lstrcmpiA(g_ed.original, g_ed.rec.name) != 0)
	{
		char oldKey[MAXNAME + 16];
		makeKeyName(g_ed.original, oldKey, sizeof(oldKey));
		if (lstrcmpiA(oldKey, g_ed.rec.key) != 0) storeDelete(oldKey);
	}
	g_ed.saved = TRUE;
	g_ed.done  = TRUE;
	DestroyWindow(hwnd);
}

static LRESULT CALLBACK editorProc(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp)
{
	switch (umsg)
	{
	case WM_CREATE:
		editorBuild(hwnd);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wp))
		{
		case IDC_BASE:
			/* A new base voice is a clean starting point. */
			if (HIWORD(wp) == CBN_SELCHANGE)
			{
				int b = (int)SendMessageA(g_ed.base, CB_GETCURSEL, 0, 0);
				int fresh[NPARAM];
				if (b >= 0 && b < NVOICE)
				{
					loadDefaults(b, fresh);
					editorShowValues(fresh);
				}
			}
			return 0;
		case IDC_TEST:
			editorReadValues();
			previewVoice(hwnd, g_ed.rec.base, g_ed.rec.val);
			return 0;
		case IDOK:
			editorOk(hwnd);
			return 0;
		case IDCANCEL:
			g_ed.done = TRUE;
			DestroyWindow(hwnd);
			return 0;
		}
		return 0;

	case WM_CLOSE:
		g_ed.done = TRUE;
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		g_ed.done = TRUE;
		return 0;
	}
	return DefWindowProcA(hwnd, umsg, wp, lp);
}

/* Returns TRUE if a voice was saved. existing == NULL means a new voice. */
static BOOL runEditor(HWND owner, const VOICEREC *existing)
{
	MSG   m;
	HWND  hwnd;
	int   w, h;
	RECT  rc;

	ZeroMemory(&g_ed, sizeof(g_ed));
	if (existing != NULL)
	{
		g_ed.rec   = *existing;
		g_ed.isNew = FALSE;
		lstrcpynA(g_ed.original, existing->name, sizeof(g_ed.original));
	}
	else
	{
		g_ed.isNew    = TRUE;
		g_ed.rec.base = 0;
		loadDefaults(0, g_ed.rec.val);
	}

	w = PANELW + 24 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 16;
	h = PANELH + 64 + 12 + 26 + 24 + GetSystemMetrics(SM_CYCAPTION) +
		2 * GetSystemMetrics(SM_CYSIZEFRAME) + 12;
	GetWindowRect(GetDesktopWindow(), &rc);

	hwnd = CreateWindowExA(WS_EX_CONTROLPARENT | WS_EX_DLGMODALFRAME,
						   "DTVMEditor",
						   existing != NULL ? "Edit voice" : "New voice",
						   WS_POPUPWINDOW | WS_CAPTION | WS_CLIPCHILDREN,
						   (rc.right - w) / 2, (rc.bottom - h) / 2, w, h,
						   owner, NULL, g_inst, NULL);
	if (hwnd == NULL) return FALSE;

	EnableWindow(owner, FALSE);
	ShowWindow(hwnd, SW_SHOW);
	SetFocus(g_ed.name);

	while (!g_ed.done && GetMessageA(&m, NULL, 0, 0))
	{
		if (!IsDialogMessageA(hwnd, &m))
		{
			TranslateMessage(&m);
			DispatchMessageA(&m);
		}
	}
	EnableWindow(owner, TRUE);
	SetForegroundWindow(owner);
	return g_ed.saved;
}

/* ------------------------------------------------------------------------- */
/* Main window                                                               */
/* ------------------------------------------------------------------------- */

#define IDC_LIST    101
#define IDC_NEW     102
#define IDC_EDIT    103
#define IDC_DELETE  104
#define IDC_EXPORT  105
#define IDC_IMPORT  106
#define IDC_STATUS  108

static HWND g_list;

static void mainRefresh(HWND hwnd, const char *select)
{
	int i, sel = 0;
	BOOL any;

	storeLoad();
	SendMessageA(g_list, LB_RESETCONTENT, 0, 0);
	for (i = 0; i < g_count; i++)
	{
		SendMessageA(g_list, LB_ADDSTRING, 0, (LPARAM)g_store[i].name);
		if (select != NULL && lstrcmpiA(select, g_store[i].name) == 0) sel = i;
	}
	any = g_count > 0;
	if (any) SendMessageA(g_list, LB_SETCURSEL, sel, 0);

	EnableWindow(GetDlgItem(hwnd, IDC_EDIT), any);
	EnableWindow(GetDlgItem(hwnd, IDC_DELETE), any);
	EnableWindow(GetDlgItem(hwnd, IDC_EXPORT), any);

	{
		char status[128];
		describeViews(status, sizeof(status));
		SetWindowTextA(GetDlgItem(hwnd, IDC_STATUS), status);
	}
}

static VOICEREC *mainSelected(void)
{
	int i = (int)SendMessageA(g_list, LB_GETCURSEL, 0, 0);
	if (i == LB_ERR || i >= g_count) return NULL;
	return &g_store[i];
}

static BOOL pickFile(HWND owner, BOOL save, const char *suggest, char *path, int cch)
{
	OPENFILENAMEA ofn;

	ZeroMemory(path, cch);
	if (suggest != NULL)
	{
		lstrcpynA(path, suggest, cch);
		lstrcatA(path, ".dtv");
	}
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = owner;
	ofn.lpstrFilter = "DECtalk voice (*.dtv)\0*.dtv\0All files (*.*)\0*.*\0";
	ofn.lpstrFile   = path;
	ofn.nMaxFile    = cch;
	ofn.lpstrTitle  = save ? "Export DECtalk voice" : "Import DECtalk voice";
	ofn.lpstrDefExt = "dtv";
	ofn.Flags       = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST |
					  (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
	return save ? GetSaveFileNameA(&ofn) : GetOpenFileNameA(&ofn);
}

static void mainNew(HWND hwnd)
{
	if (runEditor(hwnd, NULL))
		mainRefresh(hwnd, g_ed.rec.name);
}

static void mainEdit(HWND hwnd)
{
	VOICEREC copy;
	VOICEREC *v = mainSelected();
	if (v == NULL) return;
	copy = *v;
	if (runEditor(hwnd, &copy))
		mainRefresh(hwnd, g_ed.rec.name);
}

static void mainDelete(HWND hwnd)
{
	char      text[256];
	char      key[MAXNAME + 16];
	VOICEREC *v = mainSelected();
	if (v == NULL) return;

	wsprintfA(text, "Delete the custom voice %s?", v->name);
	if (!ask(hwnd, text)) return;
	lstrcpynA(key, v->key, sizeof(key));
	storeDelete(key);
	mainRefresh(hwnd, NULL);
}

static void mainExport(HWND hwnd)
{
	char      path[MAX_PATH];
	VOICEREC *v = mainSelected();
	if (v == NULL) return;
	if (!pickFile(hwnd, TRUE, v->name, path, sizeof(path))) return;
	if (!exportDtv(v, path))
		msg(hwnd, MB_ICONERROR, "The voice could not be written to that file.");
}

static void mainImport(HWND hwnd)
{
	char     path[MAX_PATH];
	char     err[256];
	char     text[256];
	VOICEREC rec;

	if (!pickFile(hwnd, FALSE, NULL, path, sizeof(path))) return;
	if (!importDtv(path, &rec, err, sizeof(err)))
	{
		msg(hwnd, MB_ICONERROR, err);
		return;
	}
	if (storeFind(rec.name) != NULL)
	{
		wsprintfA(text, "A custom voice named %s already exists. Overwrite it?", rec.name);
		if (!ask(hwnd, text)) return;
	}
	if (!saveVoice(hwnd, &rec)) return;
	mainRefresh(hwnd, rec.name);
}

static void mainBuild(HWND hwnd)
{
	int y = 12;
	int bx = 12;
	int i;

	static const struct { const char *text; int id; } buttons[] =
	{
		{ "&New...",       IDC_NEW     },
		{ "&Edit...",      IDC_EDIT    },
		{ "&Delete",       IDC_DELETE  },
		{ "E&xport...",    IDC_EXPORT  },
		{ "&Import...",    IDC_IMPORT  }
	};

	mkChild(hwnd, "STATIC", "Custom &voices:", SS_LEFT, 12, y, 200, 18, -1);
	y += 22;
	g_list = mkChild(hwnd, "LISTBOX", NULL,
					 WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
					 12, y, 400, 200, IDC_LIST);
	y += 210;

	for (i = 0; i < 3; i++)
	{
		mkChild(hwnd, "BUTTON", buttons[i].text, WS_TABSTOP | BS_PUSHBUTTON,
				bx, y, 96, 26, buttons[i].id);
		bx += 100;
	}
	y += 32;
	bx = 12;
	for (i = 3; i < 5; i++)
	{
		mkChild(hwnd, "BUTTON", buttons[i].text, WS_TABSTOP | BS_PUSHBUTTON,
				bx, y, 96, 26, buttons[i].id);
		bx += 100;
	}
	y += 36;
	mkChild(hwnd, "STATIC", "", SS_LEFT | SS_ENDELLIPSIS, 12, y + 5, 290, 18, IDC_STATUS);
	mkChild(hwnd, "BUTTON", "&Close", WS_TABSTOP | BS_DEFPUSHBUTTON,
			412 - 96, y, 96, 26, IDCANCEL);
}

static LRESULT CALLBACK mainProc(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp)
{
	switch (umsg)
	{
	case WM_CREATE:
		mainBuild(hwnd);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wp))
		{
		case IDC_LIST:
			if (HIWORD(wp) == LBN_DBLCLK) mainEdit(hwnd);
			return 0;
		case IDC_NEW:     mainNew(hwnd);        return 0;
		case IDC_EDIT:    mainEdit(hwnd);       return 0;
		case IDC_DELETE:  mainDelete(hwnd);     return 0;
		case IDC_EXPORT:  mainExport(hwnd);     return 0;
		case IDC_IMPORT:  mainImport(hwnd);     return 0;
		case IDCANCEL:
			DestroyWindow(hwnd);
			return 0;
		}
		return 0;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcA(hwnd, umsg, wp, lp);
}

/* ------------------------------------------------------------------------- */
/* Start-up                                                                  */
/* ------------------------------------------------------------------------- */

static BOOL relaunchElevated(void)
{
	char                path[MAX_PATH];
	SHELLEXECUTEINFOA   ei;

	if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) return FALSE;
	ZeroMemory(&ei, sizeof(ei));
	ei.cbSize       = sizeof(ei);
	ei.fMask        = SEE_MASK_NOCLOSEPROCESS;
	ei.lpVerb       = "runas";
	ei.lpFile       = path;
	ei.lpParameters = "/elevated";
	ei.nShow        = SW_SHOWNORMAL;
	if (!ShellExecuteExA(&ei)) return FALSE;
	if (ei.hProcess) CloseHandle(ei.hProcess);
	return TRUE;
}

static void makeFonts(void)
{
	NONCLIENTMETRICSA ncm;
	LOGFONTA          lf;

	ZeroMemory(&ncm, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	if (SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
	{
		lf = ncm.lfMessageFont;
		g_font = CreateFontIndirectA(&lf);
		lf.lfWeight = FW_BOLD;
		g_fontBold = CreateFontIndirectA(&lf);
	}
	if (g_font == NULL)     g_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (g_fontBold == NULL) g_fontBold = g_font;
}

static BOOL registerClasses(void)
{
	WNDCLASSA wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.style         = 0;
	wc.lpfnWndProc   = mainProc;
	wc.hInstance     = g_inst;
	wc.hIcon         = LoadIconA(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = "DTVMMain";
	if (!RegisterClassA(&wc)) return FALSE;

	wc.lpfnWndProc   = editorProc;
	wc.lpszClassName = "DTVMEditor";
	if (!RegisterClassA(&wc)) return FALSE;

	wc.lpfnWndProc   = panelProc;
	wc.lpszClassName = "DTVMPanel";
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	if (!RegisterClassA(&wc)) return FALSE;

	return TRUE;
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
	MSG  m;
	HWND hwnd;
	int  w, h;
	RECT rc;

	(void)prev;
	(void)show;
	g_inst = inst;

	detectViews();
	if (!canWriteTokens())
	{
		if (cmdline == NULL || strstr(cmdline, "/elevated") == NULL)
		{
			if (relaunchElevated()) return 0;
		}
		g_readOnly = TRUE;
	}

	InitCommonControls();
	g_comReady = SUCCEEDED(CoInitialize(NULL));
	makeFonts();
	if (!registerClasses())
	{
		msg(NULL, MB_ICONERROR, "The window classes could not be registered.");
		return 1;
	}

	w = 424 + 2 * GetSystemMetrics(SM_CXSIZEFRAME) + 16;
	h = 12 + 22 + 200 + 10 + 32 + 32 + 36 + 26 + 12 +
		GetSystemMetrics(SM_CYCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME);
	GetWindowRect(GetDesktopWindow(), &rc);

	hwnd = CreateWindowExA(WS_EX_CONTROLPARENT, "DTVMMain", APPNAME,
						   WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
						   WS_MINIMIZEBOX | WS_CLIPCHILDREN,
						   (rc.right - w) / 2, (rc.bottom - h) / 2, w, h,
						   NULL, NULL, inst, NULL);
	if (hwnd == NULL)
	{
		msg(NULL, MB_ICONERROR, "The main window could not be created.");
		return 1;
	}

	mainRefresh(hwnd, NULL);
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetFocus(g_list);

	if (g_readOnly)
		msg(hwnd, MB_ICONWARNING,
			"This program could not get write access to the SAPI5 voice list, so "
			"voices cannot be saved. Close it and start it again as administrator.");
	else if (g_viewCount == 0)
		msg(hwnd, MB_ICONWARNING,
			"No registered DECtalk SAPI5 engine was found, so there is nowhere to "
			"register a custom voice. Install DECtalk first.");

	while (GetMessageA(&m, NULL, 0, 0))
	{
		HWND active = GetActiveWindow();
		if (active == NULL || !IsDialogMessageA(active, &m))
		{
			TranslateMessage(&m);
			DispatchMessageA(&m);
		}
	}

	previewStop();
	if (g_comReady) CoUninitialize();
	return 0;
}
