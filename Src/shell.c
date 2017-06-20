#define _shell_c_

/***************** INCLUDES *** START******************************************/
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"

#ifdef SHELL

#pragma Ospace

typedef enum
{
    ESC_SEQ_NONE             = 0,     //
    ESC_SEQ_ESCAPE           = 0x1B,  // Escape
    ESC_SEQ_OPENSQBR         = 0x5B,  // '['
    ESC_SEQ_CMD_CURSORUP     = 0x41,  // 'A'
    ESC_SEQ_CMD_CURSORDOWN   = 0x42,  // 'B'
    ESC_SEQ_CMD_CURSORFWD    = 0x43,  // 'C'
    ESC_SEQ_CMD_CURSORBACK   = 0x44,  // 'D'
} Shell_EscSequencePart;

//type of input argument to be scanned
typedef enum    _ScanArgType
{
    SCAN_ARG_DEC,           //int32 dec
    SCAN_ARG_DEC_LONG,      //int64 dec
    SCAN_ARG_HEX,
    SCAN_ARG_HEX_LONG,
    SCAN_ARG_FLOAT,
    SCAN_ARG_STRING
}   ScanArgType, *PScanArgType;

void Shell_Delete_Argv(Shell* pShell);
extern const CmdPairStruct CmdPairs[];
/****************** INCLUDES *** END ******************************************/

#define  CHG_MOD_CHAR  ('/')            // char prefix to specify module

/*****************  Local VARIABLES *** START *********************************/
Shell   Default_Shell;
Shell*  pShell = NULL;                  // Shell context
/*****************  Local VARIABLES *** END   *********************************/

/*****************  LOCAL FUNCTION DECLARATION*** START ***********************/

/**
* @brief    Outputs the command prompt '>'
* @param    void
* @return   nothing
*/
void Shell_prompt(void);


STATIC_NO_INLINE void _Shell_DoBackspace(INT16S count)
{
    INT16S i;
    for (i = 0; i < count; i++)
    {
        if (pShell->pCmd > 0)
        {
            //fputs("\b \b", stdout);
            Shell_Printf("\b \b");
            pShell->pCmd--;
        }
    }
}


#if SHELL_COMMAND_HISTORY
STATIC_NO_INLINE INT8U _Shell_History_GetNextIndex(INT8U idx)
{
    if (idx+1 >= SHELL_HISTORY_MAX_COUNT)
        return 0;
    else
        return idx + 1;
}


STATIC_NO_INLINE INT8U _Shell_History_GetPrevIndex(INT8U idx)
{
    if (idx > 0)
        return idx - 1;
    else
        return SHELL_HISTORY_MAX_COUNT - 1;
}


STATIC_NO_INLINE void _Shell_History_SetCmdline()
{
    strncpy(pShell->mCommandString, pShell->cmdHistory[pShell->cmdHistoryRecall].cmdLine, SHELL_HISTORY_CMDLINE_LEN);
    pShell->pCmd = strlen(pShell->mCommandString);
    Shell_Printf("%s", pShell->mCommandString);
}


static void _Shell_History_Save()
{
    if (strlen(pShell->mCommandString) > 0)  // if cmd string contains some text
    {
        char* lastHistoryCmdLine = pShell->cmdHistory[_Shell_History_GetPrevIndex(pShell->cmdHistoryHead)].cmdLine;
        if (strcmp(pShell->mCommandString, lastHistoryCmdLine) != 0)  // if cmd string differs from the previous one
        {   // then save it
            char* historyCmdLine = pShell->cmdHistory[pShell->cmdHistoryHead].cmdLine;
            strncpy(historyCmdLine, pShell->mCommandString, SHELL_HISTORY_CMDLINE_LEN-1);  // strncpy() does not put '\0' to the end
            historyCmdLine[SHELL_HISTORY_CMDLINE_LEN-1] = '\0';                            // of buffer if N chars were copied
            pShell->cmdHistoryRecall = SHELL_HISTORY_INVALID_IDX;  // mark there is no walking over cmd history now
            pShell->cmdHistoryHead = _Shell_History_GetNextIndex(pShell->cmdHistoryHead);
        }
    }
}


static void _Shell_History_RestorePrev()
{
    if (pShell->cmdHistoryRecall == SHELL_HISTORY_INVALID_IDX)
        pShell->cmdHistoryRecall = _Shell_History_GetPrevIndex(pShell->cmdHistoryHead);
    else
        pShell->cmdHistoryRecall = _Shell_History_GetPrevIndex(pShell->cmdHistoryRecall);

    // if command history has not been overflown then there are some empty entries; skip them.
    if (strlen(pShell->cmdHistory[pShell->cmdHistoryRecall].cmdLine) == 0)
        pShell->cmdHistoryRecall = _Shell_History_GetPrevIndex(pShell->cmdHistoryHead);

    _Shell_History_SetCmdline();
}


static void _Shell_History_RestoreNext()
{
    if (pShell->cmdHistoryRecall == SHELL_HISTORY_INVALID_IDX)
        pShell->cmdHistoryRecall = pShell->cmdHistoryHead;
    else
        pShell->cmdHistoryRecall = _Shell_History_GetNextIndex(pShell->cmdHistoryRecall);

    // if command history has not been overflown then there are some empty entries; skip them.
    if (strlen(pShell->cmdHistory[pShell->cmdHistoryRecall].cmdLine) == 0)
        pShell->cmdHistoryRecall = 0;

    _Shell_History_SetCmdline();
}
#endif  // SHELL_COMMAND_HISTORY


/**
* @brief       Gets a line of console input
*
* @param       void
* @Returns     SHELL_INPUT_EOLINE on '\n', or
*              SHELL_INPUT_ESCAPE_SEQUENCE on escape sequence, or
*              SHELL_INPUT_CHAR on any other char.
*/
int Shell_getALine(char c)
{
#if SHELL_COMMAND_HISTORY
    static Shell_EscSequencePart escSeqPart = ESC_SEQ_NONE;
#endif  // SHELL_COMMAND_HISTORY
    char *cp  =  pShell->mCommandString;

#if SHELL_COMMAND_HISTORY
    // Escape sequences in std shell are about handling of special commands like cursor control
    // (via "arrow" keys, page-down etc), scrolling, font control etc.
    // A common escape sequence looks like the following sequence of bytes:
    // 0x1B 0x5B 0x41
    // which are the following characters respectively:
    // ESCAPE, '[', 'A'  -- this example translates to key ARROW_UP pressed on the keyboard
    if (escSeqPart == ESC_SEQ_ESCAPE && c == (char)ESC_SEQ_OPENSQBR)
    {
        escSeqPart = ESC_SEQ_OPENSQBR;
        return SHELL_INPUT_ESCAPE_SEQUENCE;
    }
    else if (escSeqPart == ESC_SEQ_OPENSQBR)  // just received char "c" must be a command in this case -- completion of the esc-sequence
    {
        _Shell_DoBackspace(pShell->pCmd);  // clear command line
        switch ((Shell_EscSequencePart)c)
        {
        case ESC_SEQ_CMD_CURSORUP:
            _Shell_History_RestorePrev();
            break;
        case ESC_SEQ_CMD_CURSORDOWN:
            _Shell_History_RestoreNext();
            break;
        }

        // any unknown command are ignored. Escape sequence must be reset here anyway
        escSeqPart = ESC_SEQ_NONE;
        return SHELL_INPUT_ESCAPE_SEQUENCE;
    }
    else if (escSeqPart != ESC_SEQ_NONE)  // no escape sequence is being translated; reset
    {
        escSeqPart = ESC_SEQ_NONE;
    }

    if (c == (char)ESC_SEQ_ESCAPE)  // ESCAPE or beginning of ESC-sequence
    {
        escSeqPart = ESC_SEQ_ESCAPE;
    }
    else
#endif   // SHELL_COMMAND_HISTORY
    
    if (c == '\b' || c == 0x7F)  // BACKSPACE or DELETE. Do backspace then
    {
        _Shell_DoBackspace(1);
    }
    else if (pShell->pCmd < MAX_COMMAND_LINE && ((c > 31 && c < 127) || c == '\r' || c == '\n'))  // Any printable char
    {
        if (c != '\n')
        {
            *(cp + pShell->pCmd) = c;
            pShell->pCmd++;
        }
        Shell_Printf("%c", c);
    }

    if (c != '\n')  // not end of line
    {
        return SHELL_INPUT_CHAR;
    }
    else   // end of line
    {
        *(cp + pShell->pCmd) = '\0';
        pShell->pCmd = 0;
        return SHELL_INPUT_EOLINE;
    }
}

/**
*@brief     Command shell task.  Processes user commands
*           and passes them off to the module command
*           handlers.  Also supports "exit" and "help".
*
*@param      pData : not used.
*@return     Must not return
*/
void Shell_task(char c)
{
    if (Shell_getALine(c) == SHELL_INPUT_EOLINE)
    {
        Shell_PerformCommandLine();
    }
}


#if 0
/** @brief read register and display the read back value
 *   This command is invoked by Shell commandprocessor upon receive command "regr [addr in hex]"
 * @param ps    : pointer to shell structure
 * @param idx   : pointer to index of first argument token
 * @param cmd   : command
 */
INT32U Shell_cmdRegR(Shell *ps, int *mIndex, char *cmd)
{
    unsigned int addr = 0;

    if (Shell_ScanHexSh(ps, mIndex, &addr) == ERR_NO_ERROR)
    {
        Shell_Printf("%X\n",*((unsigned long*)addr));
        return ERR_NO_ERROR;
    }
    else
    {
        Shell_Printf ("Invalid address.\n");
        return ERR_INVALID_PARAM;
    }
}

/** @brief write register at specified address with supplied value
* this command is invoked by Shell command processor upon receiving command "regw [addr in hex] [val in hex]"
 * @param ps    : pointer to shell structure
 * @param idx   : pointer to index of first argument token
 * @param cmd   : command
*/
INT32U Shell_cmdRegW(Shell *ps, int *mIndex, char *cmd)
{
    unsigned int addr = 0;
    unsigned int value = 0;

    if (Shell_ScanHexSh(ps, mIndex, &addr) != ERR_NO_ERROR)
    {
        Shell_Printf ("Invalid address.\n");
        return ERR_INVALID_PARAM;
    }

    if (Shell_ScanHexSh(ps, mIndex, &value) != ERR_NO_ERROR)
    {
        Shell_Printf ("Invalid value.\n");
        return ERR_INVALID_PARAM;
    }
    Shell_Printf("%X = %X\n", addr, value);

    *((volatile unsigned int *)addr) = value;

    return ERR_NO_ERROR;
}
#endif


/**
* Function:    Shell_ScanSh
*
* @brief       Scans an argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       ps  :               pointer to "the" shell object
* @Param       idx :               parameter index
* @Param       ret :               result location
* @Param       type:               indicates expected return type
* @Parm        len :               max. string length (when type='s')(mem size n+1))
*
* @Returns     ERR_xxx code
*/
INT32U Shell_scanSh (Shell *ps, int* idx, void* ret, ScanArgType type, int len)
{
    INT32U  rval = ERR_NO_ERROR;         // assume sucess
    char    *cp;
    char    *start, *end;
    int     isResult = 0;

    if (ps != NULL)
    {
        if (*idx >= ps->mNumTokens)
        {
            // Too few parameters entered
            rval = ERR_TRDB_FAIL;
        }
        else
        {
            start = ps->mpToken[*idx];
            end   = start + strlen(ps->mpToken[*idx]);

            // Validate parameter
            for (cp = start; cp < end; ++cp)
            {
                if (type == SCAN_ARG_DEC || type == SCAN_ARG_DEC_LONG)
                {
                    isResult = isdigit((unsigned char)*cp) || (*cp == '-');
                }
                else if (type == SCAN_ARG_HEX || type == SCAN_ARG_HEX_LONG)
                {
                    isResult = isxdigit((unsigned char)*cp);
                }
#ifdef SHELL_SCAN_FLOAT
                else if (type == SCAN_ARG_FLOAT)
                {
                    isResult = isfdigit((unsigned char)*cp);
                }
#endif
                else if (type == SCAN_ARG_STRING)
                {
                    isResult = isprint((unsigned char)*cp);
                }

                if (!isResult)
                {
                    rval = ERR_TRDB_FAIL;
                    break;
                }
            }

            if (rval == ERR_NO_ERROR)
            {
                if (type == SCAN_ARG_DEC)
                {
                    *(int *)ret = atoi(start);     // Return result to caller
                }
                if (type == SCAN_ARG_DEC_LONG)
                {
                    *(long long *)ret = atoll(start);
                }
                else if (type == SCAN_ARG_HEX)
                {
                    *(unsigned long *)ret = strtoul(start, NULL, 16);
                }
                else if (type == SCAN_ARG_HEX_LONG)
                {
                    *(long long *)ret = strtoll(start, NULL, 16);
                }
#ifdef SHELL_SCAN_FLOAT
                else if (type == SCAN_ARG_FLOAT)
                {
                    *(float*) ret = strtod(start,NULL);
                }
#endif
                else if (type == SCAN_ARG_STRING)
                {
                    strncpy((char*)ret, start, len);
                }

                // Return result to caller
                ++(*idx);               // Advance the parameter index
            }
        }
    }
    return rval;
}

#ifdef OLD_SHELL_SCAN
/**
* Function:    Shell_Scan
*
* @brief       Scans an argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       cmd :               cmd string
* @Param       idx :               parameter index
* @Param       num :               result location
* @Parm        n   :               max. string length (mem size n+1)
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_scan(char* cmd, int* idx, void* ret, char type, int n)
{
    return Shell_scanSh (pShell, idx, ret, type, n);
}
#endif

/******************* IMPLEMENTATIONS (GLOBAL FNS) ******************************/

/**
* Function:    Shell_scanstr
*
* @brief       Scans a string argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Note
*              Note that the string is copied to the caller's
*              context as opposed to passing a pointer to the
*              string.
*
* @Param      cmd                 cmd string
* @Param      idx                 parameter index
* @Param      result              resultant string location
* @Param      n                   max string len(i.e. mem size n+1)
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanStr(char* cmd, int* idx, char* result, int n)
{
    return Shell_scanSh(pShell, idx, result, SCAN_ARG_STRING, n);
}

/**
* Function:    Shell_ScanStrSh
*
* @brief       Scans a string argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Note
*              Note that the string is copied to the caller's
*              context as opposed to passing a pointer to the
*              string.
*
* @Param      cmd                 cmd string
* @Param      idx                 parameter index
* @Param      result              resultant string location
* @Param      n                   max string len(i.e. mem size n+1)
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanStrSh(Shell *ps, int* idx, char* result, int n)
{
    return Shell_scanSh(ps, idx, result, SCAN_ARG_STRING, n);
}

/**
* Function:    Shell_ScanInt
*
* @brief       Scans an integer argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       cmd :               cmd string
* @Param       idx :               parameter index
* @Param       num :               result location
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanInt(char* cmd, int* idx, int* num)
{
    return Shell_scanSh(pShell, idx, num, SCAN_ARG_DEC, 0);
}

/**
* Function:    Shell_ScanIntSh
*
* @brief       Scans an integer argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       cmd :               cmd string
* @Param       idx :               parameter index
* @Param       num :               result location
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanIntSh(Shell *ps, int* idx, int* num)
{
    return Shell_scanSh(pShell, idx, num, SCAN_ARG_DEC, 0);
}

/**
* Function:    Shell_ScanIntLong
*
* @brief       Scans an integer argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       cmd :               cmd string
* @Param       idx :               parameter index
* @Param       num :               result location
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanIntLongSh(Shell *ps, int* idx, long long* num)
{
    return Shell_scanSh(ps, idx, num, SCAN_ARG_DEC_LONG, 0);
}

/**
* Function:    Shell_scanhex
*
* @brief       Scans a hex argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       cmd :                cmd string
* @Param       idx :                parameter index
* @Param       num :                result location
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanHex(char* cmd, int* idx, unsigned int* num)
{
    return Shell_scanSh(pShell, idx, num, SCAN_ARG_HEX, 0);
}

/**
* Function:    Shell_ScanHexSh
*
* @brief       Scans a hex argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       cmd :                cmd string
* @Param       idx :                parameter index
* @Param       num :                result location
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanHexSh(Shell* ps, int* idx, unsigned int* num)
{
    return Shell_scanSh(ps, idx, num, SCAN_ARG_HEX, 0);
}

/**
* Function:    Shell_ScanHexLong
*
* @brief       Scans a hex argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       cmd :                cmd string
* @Param       idx :                parameter index
* @Param       num :                result location
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanHexLongSh(Shell* ps, int* idx, unsigned int* num)
{
    return Shell_scanSh(ps, idx, num, SCAN_ARG_HEX_LONG, 0);
}

#ifdef SHELL_SCAN_FLOAT
/**
* Function:    Shell_ScanFloat
*
* @brief       Scans an float argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*
* @Param       cmd :               cmd string
* @Param       idx :               parameter index
* @Param       num :               result location
*
* @Returns     ERR_xxx code
*/
unsigned long Shell_ScanFloat(char* cmd, int* idx, float* num)
{
    return Shell_scanSh(pShell, idx, num, SCAN_ARG_FLOAT, 0);
}
#endif

/**
 * @brief Count and store possible integer numbers from argument tokens.
 * @params  ps : pointer to active shell
 * @params  idx: pointer to index of first argument token
 *
 * @return  number of integer parameters, starting from first argument token
 */
int  Shell_IntArgC(Shell *ps, int idx)
{
    int i;
    for (i=0; i<MAX_INT_ARGV; i++)
    {
        if (Shell_ScanHexSh(ps, &idx, (INT32U*) &(ps->mIntArgV[i])) != ERR_NO_ERROR)
            break;
    }
    ps->mIntArgC = i;
    return i;
}


/**
* Function:    Shell constructor
*
* @brief       constructor
*
* @param       void
* @Returns     void
*/
INT32U Shell_Create(void)
{
    memset(&Default_Shell, 0, sizeof(Default_Shell));
#if SHELL_COMMAND_HISTORY
    Default_Shell.cmdHistoryRecall = SHELL_HISTORY_INVALID_IDX;
#endif
    Default_Shell.mNumTokens       = -1;
    Default_Shell.mIndex           = -1;
    pShell = &Default_Shell;
    return 0;
}

/**
 *  @brief dispatch a command according to a command table.
 *
 *  @param ps    : pointer to shell structure
 *  @param idx   : pointer to index of first argument token
 *  @param cmd   : command
 *  @param pCmdTbl: point to command table.
 *
 *  @return TRUE if command is matched and is executed; FALSE otherwise.
 */
BOOL Shell_DispatchCmdTbl(Shell *ps, int* idx, char* cmd, CmdPairStruct *pCmdTbl)
{
    while (pCmdTbl->mCmdStr != NULL)
    {
        if (strcmp(cmd, pCmdTbl->mCmdStr) == 0)
        {
            Shell_IntArgC(ps, *idx);
            pCmdTbl->mCmdProc(ps, idx, cmd);
            return TRUE;
        }
        pCmdTbl++;
    }
    
    Shell_Printf("Invalid command \"%s\"\n", pShell->mCommandString);
    return FALSE;
}


/**
 * @brief
 * @param ps    : pointer to shell structure
 * @param idx   : pointer to index of first argument token
 * @param cmd   : command
 */
void Shell_Delete_Argv(Shell* pShell)
{
    int i=0;

    pShell->mIntArgC = 0;
    memset(pShell->mCommandString, 0, MAX_COMMAND_LINE);
    for(i=0; i<MAX_INT_ARGV; i++)
    {
        pShell->mIntArgV[i] = 0;
    }
}

/**
* @brief Outputs the command prompt
* @param      void
* @return    nothing
*/
void Shell_prompt(void)
{
    Shell_Printf("=>");
}


/**
* @brief Convert letters in the string to lower case
* @param  str    : string to be converted to lower
* @return    nothing
*/
void strToLower(char *str)
{
    int maxnum = MAX_COMMAND_LINE;

    while (*str && maxnum > 0)
    {
        if (*str >= 'A' && *str <= 'Z')
            *str+=32;
        str ++;
        maxnum--;
    }
}

/**
* @brief Outputs the command prompt
* @return    nothing
*/
void Shell_PerformCommandLine(void)
{
    char*       pCommand = 0;

    //Shell_Printf("\r");    // '\n' echoed in low level already.

    strToLower(pShell->mCommandString);

    // The strtok() processing inserts NULL characters for
    // delimiters.  So, copy the command string to a working
    // area.

    strncpy(pShell->mWorkingString, pShell->mCommandString, MAX_COMMAND_LINE);

    // Parse the tokens from the working command line.
    // Valid parameter delimiters are space, dash, and
    // comma.  These delimiters are all stripped from the
    // command line.

    pShell->mNumTokens = 0;
    pShell->mpToken[pShell->mNumTokens] = strtok(pShell->mWorkingString, " ");

    while (pShell->mpToken[pShell->mNumTokens] && pShell->mNumTokens < MAX_TOKENS)
    {
        ++(pShell->mNumTokens);
        pShell->mpToken[pShell->mNumTokens] = strtok((char *)NULL, " ,");
    }

    pCommand = pShell->mpToken[0];

    // try to match command
    if (pCommand && pShell->mNumTokens)
    {
        pShell->mIndex = 1;
#if SHELL_COMMAND_HISTORY
        if (Shell_DispatchCmdTbl(pShell, (int*) &(pShell->mIndex), pCommand, (CmdPairStruct*)CmdPairs) == TRUE)
        {
            _Shell_History_Save();
        }
#else  // SHELL_COMMAND_HISTORY
        Shell_DispatchCmdTbl(pShell, (int*) &(pShell->mIndex), pCommand, (CmdPairStruct*)CmdPairs);
#endif  // SHELL_COMMAND_HISTORY
    }

    Shell_Delete_Argv(pShell);
    Shell_prompt();
}

/**
* @brief Outputs the command prompt
* @param      cmdLine     - command line string that will change current command line. If cmdLine == NULL then do nothing.
* @return    nothing
*/
void Shell_SetCommandLine(char* cmdLine)
{
    if (cmdLine)
    {
        unsigned int len = 0;

        cmdLine[MAX_COMMAND_LINE - 1] = '\0';

        len = strlen(cmdLine);

        if (len >= MAX_COMMAND_LINE)
        {
            len = MAX_COMMAND_LINE - 1;
        }

        memcpy ((void*)pShell->mCommandString, (void*)cmdLine, len);
        pShell->mCommandString[len] = '\0';
        pShell->pCmd                = 0;
    }
}

#endif //SHELL
