#ifndef _CLI_H_
#define _CLI_H_


typedef void (*CliCmdFunc) ( s32, char ** );


typedef struct CLICmds_st
{
        const char           *Cmd;
        CliCmdFunc          CmdHandler;
        const char           *CmdUsage;
        
} CLICmds, *PCLICmds;




s32  Cli_RunCmd(char *CmdBuffer);
void Cli_Init(s32 argc, char *argv[]);
void Cli_Start(void);
void Cli_Task( void *args );

#endif /* _CLI_H_ */

