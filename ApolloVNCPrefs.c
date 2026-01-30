#include <exec/types.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#ifndef IPTR
typedef unsigned long IPTR;
#endif

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) (((ULONG)(a)<<24)|((ULONG)(b)<<16)|((ULONG)(c)<<8)|(ULONG)(d))
#endif

struct Library *MUIMasterBase = NULL;

/* Deux IDs distincts */
#define ID_CONNECT 1001
#define ID_SAVE    1002

static void SavePrefs(STRPTR host, STRPTR user, STRPTR pass) {
    BPTR fh = Open("ENVARC:ApolloVNC.prefs", MODE_NEWFILE);
    if (fh) {
        FPuts(fh, host); FPuts(fh, "\n");
        FPuts(fh, user); FPuts(fh, "\n");
        FPuts(fh, pass); FPuts(fh, "\n");
        Close(fh);
    }
}

static void LoadPrefs(STRPTR host, STRPTR user, STRPTR pass) {
    BPTR fh = Open("ENVARC:ApolloVNC.prefs", MODE_OLDFILE);
    if (fh) {
        FGets(fh, host, 128); host[strcspn(host, "\r\n")] = 0;
        FGets(fh, user, 128); user[strcspn(user, "\r\n")] = 0;
        FGets(fh, pass, 128); pass[strcspn(pass, "\r\n")] = 0;
        Close(fh);
    }
}

int main(void) {
    Object *app, *win, *strHost, *strUser, *strPass;
    Object *btnConnect, *btnSave; 
    
    char bufHost[128] = "", bufUser[128] = "", bufPass[128] = "";
    char cmd[512];
    STRPTR h, u, p;
    ULONG signals = 0;
    ULONG res;
    BOOL running = TRUE;

    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 19))) return 20;

    LoadPrefs(bufHost, bufUser, bufPass);

    app = ApplicationObject,
        MUIA_Application_Title,   (IPTR)"ApolloVNC Prefs",
        MUIA_Application_Base,    (IPTR)"APVNC",
        
        SubWindow, win = WindowObject,
            MUIA_Window_Title, (IPTR)"ApolloVNC Prefs",
            MUIA_Window_ID,    (IPTR)MAKE_ID('V','N','C','L'),
            MUIA_Window_RootObject, VGroup,
                MUIA_Background, MUII_GroupBack,

                /* Champs de saisie */
                Child, GroupObject,
                    MUIA_Group_Columns, 2,
                    
                    Child, TextObject, MUIA_Text_Contents, (IPTR)"Host IP:", End,
                    Child, strHost = StringObject, MUIA_String_Contents, (IPTR)bufHost, End,
                    
                    Child, TextObject, MUIA_Text_Contents, (IPTR)"User:", End,
                    Child, strUser = StringObject, MUIA_String_Contents, (IPTR)bufUser, End,
                    
                    Child, TextObject, MUIA_Text_Contents, (IPTR)"Password:", End,
                    Child, strPass = StringObject, 
                        MUIA_String_Contents, (IPTR)bufPass,
                        MUIA_String_Secret, TRUE,
                    End,
                End,

                /* Groupe horizontal pour les boutons */
                Child, GroupObject,
                    MUIA_Group_Horiz, TRUE, /* Alignement horizontal */
                    Child, btnConnect = MUI_MakeObject(MUIO_Button, (IPTR)"_Connect"),
                    Child, btnSave    = MUI_MakeObject(MUIO_Button, (IPTR)"_Save"),
                End,
            End,
        End,
    End;

    if (!app) { CloseLibrary(MUIMasterBase); return 20; }

    /* Notifications - Fermeture */
    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2, 
             MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
             
    /* Notifications - Bouton Connect */
    DoMethod(btnConnect, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, 
             MUIM_Application_ReturnID, ID_CONNECT);

    /* Notifications - Bouton Save */
    DoMethod(btnSave, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, 
             MUIM_Application_ReturnID, ID_SAVE);

    set(win, MUIA_Window_Open, TRUE);

    while (running) {
        res = DoMethod(app, MUIM_Application_NewInput, (IPTR)&signals);
        
        if (res == MUIV_Application_ReturnID_Quit) {
            running = FALSE;
        }
        else if (res == ID_CONNECT) {
            /* Action Connect : On lit les champs et on lance */
            get(strHost, MUIA_String_Contents, &h);
            get(strUser, MUIA_String_Contents, &u);
            get(strPass, MUIA_String_Contents, &p);

            sprintf(cmd, "ApolloVNC %s %s %s", h, u, p);
            Execute((STRPTR)cmd, 0, 0);
        }
        else if (res == ID_SAVE) {
            /* Action Save : On lit les champs et on sauve */
            get(strHost, MUIA_String_Contents, &h);
            get(strUser, MUIA_String_Contents, &u);
            get(strPass, MUIA_String_Contents, &p);

            SavePrefs(h, u, p);
            
            /* Petit feedback visuel (optionnel) : beep */
            DisplayBeep(NULL); 
        }

        if (running && signals) {
            signals = Wait(signals | SIGBREAKF_CTRL_C);
            if (signals & SIGBREAKF_CTRL_C) running = FALSE;
        }
    }

    MUI_DisposeObject(app);
    CloseLibrary(MUIMasterBase);
    return 0;
}
