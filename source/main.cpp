/******************************************************************************/
/*           ��������� ��� ������� ������� ��� ������ ���������               */
/* -------------------------------------------------------------------------- */
/* ������ ������ ����� ���� 2009 ����� ������ ������ ������������             */
/* ����� ����������. ����� ������� � ���� ������ ��������, ���� �������� �    */
/* � ����������:        pavel@gromovikov.name.                                */  
/* ������ �������� ��� ���������� �� ����� ����� ��������� � ��� �� ������    */
/* ��������� �� ���������� ������, ����� � � ��������, �� ��������� ������    */
/* � �������������.                                                           */  
/* ������ ��������� ������ ���������� "phones" � ������ � ����� � ����������  */
/* ����� ��������� ������ ���������� "messages" � ������ � ����� � ���������� */
/*   ��������� - �������� ��������� ��� ��� ���������������� ��������         */
/*   ���������� ��������� ��. 18 ������������ ������ � �������!               */
/*                                                                            */
/******************************************************************************/

#include <cstdlib>
#include <iostream>
#include <windows.h>
 
#define MAX_PDU_LEN 176
#define MAX_PHONE_LEN 11
#define MIN_PHONE_LEN 11
#define MAX_UCS2_LEN 67  //�� 3 ������� ������, ����� ���� UDH
#define COM_BUFF_LEN   255   
#define MAX_PARTS_COUNT 10
//#define SHOW_AT_DIALOG

using namespace std;
HANDLE FindPort();
COMMTIMEOUTS  ComTimeOutsOld;
int IssueCommand (HANDLE port, char* Command, char* Answer,byte OK_needed=TRUE);
int commitPDU (HANDLE port, char* message);
int pdu_encode (char *PDU,
                char * phone,
                char * message,
                unsigned char CSMSID=0,
                unsigned char CSMSQ=0,
                unsigned char CSMSN=0);

int SendLongMessage (HANDLE port, char* message, char * phone);
char *str_CR ="\r";
char *str_LF ="\n";

int erase_n_printf( const char * format, ... ) {
	va_list args;
    va_start( args, format ); // retrieve the variable arguments
	printf ("\r                                       \r");
    vprintf(format, args); 
}


/******************************************************************************/
/*                         ������ ��� �� ������                               */
/* -------------------------------------------------------------------------- */
/*   ��������� - �������� ��������� ��� ��� ���������������� ��������         */
/*   ���������� ��������� ��. 18 ������������ ������ � �������!               */
/*                                                                            */
/******************************************************************************/
//  
//
int main(int argc, char *argv[]) {
   
 char AnswBuf[COM_BUFF_LEN];
 char tmp2[64];
//������ ������ ���� � cp1251 � �� ������ MAX_UCS2_LEN
 char message[32768]="";
 char phones [32768]="";
 char number [MAX_PHONE_LEN+1];                             
 HANDLE port = 0;
 char message_file_name [256]="message.txt";
 HANDLE message_file=0;
 char phones_file_name [256]="numbers.txt";
 HANDLE phones_file=0;
 DWORD  written=0;   
 unsigned i=0; 
 unsigned phones_size;
 
 //���������� ������ ����������
     int ctr;
     int mctr=0;
     int nctr=0;
     char arg_error=0;
     
     for( ctr=1; ctr < argc; ctr++ )
     { arg_error=1;
       if (!strcmp (argv[ctr],"-m") ||!strcmp (argv[ctr],"-M") ) {
            if (ctr>=argc || argv[ctr+1][0]=='-' 
            || mctr++ || strlen (argv[ctr+1])>=sizeof(message)) {
                
                printf ("Incorrect message parameters \n");
                break;
            }
            strcpy (message, argv[ctr+1]);
        }
        if (!strcmp (argv[ctr],"-n") ||!strcmp (argv[ctr],"-N") ) {
            if (ctr>=argc || argv[ctr+1][0]=='-'
            || nctr++ || strlen (argv[ctr+1])>=sizeof(phones)) {
                printf ("Incorrect phone number(s) parameters \n");
                break;
            }
            strcpy (phones, argv[ctr+1]);
            strcat (phones, " ");
            phones_size=strlen(phones);
        }
        if (!strcmp (argv[ctr],"-mf") ||!strcmp (argv[ctr],"-MF") ) {
            if (ctr>=argc || argv[ctr+1][0]=='-'
            || mctr++ || strlen (argv[ctr+1])>=sizeof(message_file_name)) {
                printf ("Incorrect message parameters \n");
                break;
            }
            strcpy (message_file_name, argv[ctr+1]);
        }
        if (!strcmp (argv[ctr],"-nf") ||!strcmp (argv[ctr],"-NF") ) {
            if (ctr>=argc || argv[ctr+1][0]=='-'
            || nctr++ || strlen (argv[ctr+1])>=sizeof(phones_file_name)) {
                printf ("Incorrect phone number(s) parameters \n");
                break;
            }
            strcpy (phones_file_name, argv[ctr+1]); 
        }
        arg_error=0;                   
     }
 if (arg_error)  goto END;
 
 //���� �������
 printf ("Long SMS sender with Cyrillic (Russian) support \n");
 printf ("By Pavel V. Gromovikov pavel@gromovikov.name \n\n");
 printf ("Please notice that sending not demanded sms-ads\n");
 printf ("is illegal in many countries including Russia...\n\n");
 printf ("Usage: sms_sender [-m \"your_text\" | -mf filename] [-n \"numbers\" | -nf filename]\n");
 printf ("If no \"-m\" or \"-mf\" optins are defined then text from messages.txt will be sent\n");
 printf ("If no \"-n\" or \"-nf\" optins are defined then numbers.txt will be used as a list. \n");
 printf ("Message must have cp1251 encoding for russian text to be sent correctly.\n");
 printf ("Any non-numerical symbol may be used as numbers delimiter in the list.\n\n");
 printf ("Ex: sms_sender -m \"Hi, happy fools' day\" -nf fools_numbers.csv \n");
 printf ("    sms_sender -mf for_Paha_and_Gera.txt -n \"75559720854 75556220727\" \n");
 printf ("    sms_sender -n \"75559720899\" \n");
 printf ("    message from message.txt will be sent \n");
 printf ("    sms_sender -n \"75559720899\" \n");
 printf ("        (text from message.txt will be sent to +75559720899) \n");
 printf ("    sms_sender -m \"You're in my list, fellow\" \n");
 printf ("        (all fellows from numbers.txt will then know they are in the list)\n\n");
 
 port = FindPort();
 if (!port) {
    printf ("Sorry, cant find any AT-compatible device.\n"); 
    printf ("If you use BT, authorize your PC in PHONE settings. \n");
    goto END;
 }
 

 
 
 if (!message[0]) {
    //��������� ���� ���������
    message_file = CreateFile(message_file_name, GENERIC_READ, 
                               0, 0, OPEN_EXISTING, 0, 0);
    if (message_file == INVALID_HANDLE_VALUE) {
        printf("Failed to open message file\n");
	   goto END;
    }
    ReadFile(message_file, message, 32768, &written, 0);
	if (written <= 0 || written==32768) {
	   printf("Failed to read message file - empty or too big\n");
	   goto END;
    }
    CloseHandle(message_file);
    message_file=0;
    message[written]=0;
 }

 if (!phones[0]) { 
    //��������� ���� � ����������
    phones_file = CreateFile(phones_file_name, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if (phones_file == INVALID_HANDLE_VALUE) {
       printf("Failed to open numbers file\n");
       goto END;
    }
    ReadFile(phones_file, phones, 32768, &written, 0);
    if (written <= 0 || written==32768) {
        printf("Failed to read numbers file - empty or too big\n");
        goto END;
    }
    phones[written]=' ';
    phones_size=written+1;
    CloseHandle(phones_file);
    phones_file=0;
 }
 
 //�����������    
 if   (IssueCommand (port,"AT+CMGF=0",AnswBuf,1)) {
    printf ("Setting PDU mode failed. Exiting\n");
    goto END;
 }
   
 //������� �� ������ ���������
 for (i=0;i<phones_size;i++) {
    if ((unsigned char)phones[i]>=0x30  && (unsigned char)phones[i]<=0x39){
        strncat (number, phones+i,1);
    }
    else if (strlen (number)>=MIN_PHONE_LEN && strlen (number)<=MAX_PHONE_LEN){
        SendLongMessage (port, message, number);
        strcpy (number,"");
    }
    else  strcpy (number,"");
 }        


END:
 //����� � ������ �� �����
 if (port) {
    // ������ ����� ��� �������� ��������� � ������� ���, ���� ������
    SetCommTimeouts(port, &ComTimeOutsOld);
	CloseHandle(port); 
 }
 if (message_file) {
	CloseHandle(message_file);
 }
 if (phones_file) {
	CloseHandle(phones_file);
 }
    system("PAUSE");
    return EXIT_SUCCESS;
}

/******************************************************************************/
/*                     ������� AT-������� (�����) �� ���.                     */
/* -------------------------------------------------------------------------- */
/* �������� ����-�������������� ������ ������� � ���������� ����� ��          */
/* ��������� ������������ ��������� � ��������� ����� � �������� �� �����     */
/* Arguments: port  - HANDLE of the s. port having the cellular device on it  */
/*            Command - null-terminated sequence of bytes to send to port.    */
/*            Answer  - dest pointer for data recieved from cell. device      */
/*            OK_needed  - defines whether OK is expected as an answer, or not*/
/* Returns:   0 if succeded                                                   */
/*            1 if failed                                                     */
/******************************************************************************/
/******************************************************************************/

int IssueCommand (HANDLE port, char* Command, char* Answer, byte OK_needed)
 {
 DWORD db=0;                          //���������� ��������/���������� ����
 char tmp2[COM_BUFF_LEN];
 char *tmp = tmp2;                         //����� �������/������
 unsigned int CommandSize;                 //������ ������������ �������

 if (strlen (Command)+4 > COM_BUFF_LEN)
    {return 1;}                            //���������� ������� ��� ��...
 strcpy (tmp,Command);
 strcat (tmp, str_CR);
 CommandSize =  strlen (tmp);              //������������ ������� ��������


#ifdef SHOW_AT_DIALOG
/*��� ������� ��������� ��� ���������� ��� ��������� �� ������� ��������*/
  unsigned int i=0;
  printf ("\n    We said:[ ");
  while (i<CommandSize) printf ("%02X ",tmp[i++]);
  printf ("] - ");
  i=0;
  while (tmp[i]!=0 && i<CommandSize) {
    if (tmp[i]==0x0D) printf ("\\r");
    else if (tmp[i]==0x0A) printf ("\\n");
    else printf ("%c",tmp[i]);
    i++;
    }
  printf (" (%d bytes) \n",CommandSize);
#endif

 WriteFile(port, tmp, CommandSize, &db, 0);//������� �������
 memset (tmp,0,sizeof (tmp2));              //��������� �����
 db=0;
 ReadFile (port, tmp, sizeof(tmp2)-1, &db, 0);//��������� �����

#ifdef SHOW_AT_DIALOG
/*��� ������� ��������� ��� ���������� ��� ���������  ������� ����� ���*/
 i=0;
 printf ("\n    Phone said:[ ");
 while (i<db) printf ("%02X ",tmp[i++]);
 printf ("] - ");
 i=0;
 while ( i<db) {
    if (tmp[i]==0x0D) printf ("\\r");
    else if (tmp[i]==0x0A) printf ("\\n");
    else printf ("%c",tmp[i]);
    i++;
    }
 printf (" (%d bytes) \n",db);
#endif
 strcpy (Answer,tmp);
 if (!db || (OK_needed && !strstr (tmp,"OK"))) return 1;


 Sleep (100);//�� ����� ������� ������� ������.
 return 0;
}

/******************************************************************************/
/*                          �������  PDU �� ���                               */
/* -------------------------------------------------------------------------- */
/*   ���������� ���, ��� ������ 0D ���� �������� � ����� ctl-Z  (1A),         */
/*   � ��� ��� �������� ������ ����� ������ ������������ ������� �����        */
/*   (����� ������� ���������)                                                */
/* -------------------------------------------------------------------------- */
/* Arguments: port  - HANDLE of the s. port having the cellular device on it  */
/*            message - null-terminated sequence of bytes to send to port.    */
/*                      sequence will be finished with Ctrl-Z. OK is expected */
/*                      as an answer, after Ctrl-Z is sent.                   */
/* Returns:   0 if succeded                                                   */
/*            1 if failed                                                     */
/******************************************************************************/

int commitPDU (HANDLE port, char* message){
 DWORD dbc;
 unsigned char OK=FALSE;
 char tmp[64];

 char sequence [strlen(message)+2];//����� � �������� - PDU+CtrlZ+0
 sprintf (sequence,"%s%c",message,0x1a);
#ifdef SHOW_AT_DIALOG
 printf ("\n Sending %s as PDU\n",sequence);
#endif
 WriteFile(port, sequence, strlen(sequence), &dbc, 0);//������� �������� � Ctrl-Z
 //�������� ������ ������ 6
 for (int i=0;i<12;i++){
    Sleep (500);
    dbc=0;
    ReadFile (port, tmp, sizeof (tmp)-1, &dbc, 0);
    if (!dbc) continue;
    if (strstr (tmp,"OK")) OK=TRUE;
    break;
    }
    if (OK) return 0;
    return 1;
}




/******************************************************************************/
/*                          ����� �����                                       */
/* -------------------------------------------------------------------------- */
/*        �������� � ������ ��������� ���� � �������� AT - �������            */
/*        ���� �������� ����� - ������� ��� ����� �������                     */  
/*        ���� � ���� �� ����� ����������� ����� - ����������.                */
/* -------------------------------------------------------------------------- */
/* Returns:   HANDLE of the ser. port having AT-command compatible dev. on it */
/*            0 if failed                                                     */
/******************************************************************************/

HANDLE FindPort() {
    
 char tmp[64];
 char AnswBuf[COM_BUFF_LEN];
 char model [255];
 int num = 0;
 unsigned char phonefound = FALSE;
 DWORD db;
 COMMTIMEOUTS  CTO;
 HANDLE port;   
 printf ("Searching for a phone...\n");
 //���������� ����� 0-255
 for(int i = 0; i < 255; i++) {
    sprintf(tmp,"\\\\.\\COM%d ", i);
    //������� �������
    port = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE,
            0,NULL,OPEN_EXISTING,0,NULL);
	if (port == INVALID_HANDLE_VALUE) continue;
    printf ("Port COM%d exists. Any phone here? ", i);
    //��������� �������� ��������� �����
    GetCommTimeouts(port, &ComTimeOutsOld);
    memcpy (&CTO, &ComTimeOutsOld,sizeof(COMMTIMEOUTS));
    //��������������� ���� ��� ����
    CTO.ReadIntervalTimeout=30;
    CTO.ReadTotalTimeoutMultiplier=0;
    CTO.ReadTotalTimeoutConstant=1000;
    SetCommTimeouts(port, &CTO);
    //�������� CTRL-Z (�� ������ ���� ����� ����� � �������� PDU)
    //� ATE0 (��������� ��� � ��������� ����������� �� �����)
    //���� ������ ����� �� ��� ���� �� ERROR - ������� ��� ����� ������� 
    if  (!IssueCommand (port,"\032ATE0",AnswBuf,1)) 
        {phonefound = TRUE; break;}                         
    if (strstr (AnswBuf,"ERROR") && 
           (!IssueCommand (port,"\032ATE0",AnswBuf,1)))
        {phonefound = TRUE; break;} 
    printf ("No...\n");
    //��������������� �������� ��������� ����� ���� �� ����� ��������
    SetCommTimeouts(port, &ComTimeOutsOld);
	CloseHandle(port); 
    }
    if (!phonefound) return 0;
    //������� ������: �������� � �������� ��� - ��������
    strcpy (model, "Found ");
    if  (IssueCommand (port,"AT+CGMI",AnswBuf,1)) return port; 
    sscanf (AnswBuf,"\r\n%s\r%*s",tmp);
    strcat (model, tmp); strcat (model, " ");    
    if  (IssueCommand (port,"AT+CGMM",AnswBuf,1)) return port;
    sscanf (AnswBuf,"\r\n%s\r%*s",tmp);
    strcat (model, tmp);   strcat (model, "\n");
    printf (model);
    return port;
}


/******************************************************************************/
/*                        �������� ���������                                  */
/* -------------------------------------------------------------------------- */
/*          ������ ��������� (���� ����) � �������� ��� �����                 */
/* -------------------------------------------------------------------------- */
/* Arguments: port  - HANDLE of the s. port having the cellular device on it  */
/*            message - string to send. Will be sent in parts if needed       */
/*            phone   - phone number to deliever the message to               */
/* Returns:   0 if succeded                                                   */
/*            1 if failed                                                     */
/******************************************************************************/

int SendLongMessage (HANDLE port, char* message, char * phone)
{
 unsigned char message_ID;
 unsigned char parts_count;
 unsigned char part_number;
 char tmp [64];
 char tmp2 [64];
 char part [MAX_UCS2_LEN+1];
 char PDU [2*MAX_PDU_LEN+1];          //PDU ��� �������� ����� AT+CMGS
 unsigned char PDUlen=0;                       //����� PDU ��� AT+CMGS
 unsigned int message_len = strlen(message);

 printf ("Sending to %s\n",phone);
  //����� ����� ���� ��������� ������� �������
 if (message_len >(MAX_PARTS_COUNT*MAX_UCS2_LEN)){
    printf (" Message is too long \n",phone);
    return 1;
    }
 //����������� "���������� �����" ��� ���� ������ ���������
 srand ( time(NULL) );
 message_ID = rand() % 0xFF + 1;
 //������� ����� ������?
 parts_count = (message_len/MAX_UCS2_LEN )+((message_len % MAX_UCS2_LEN)? 1:0);

 for (part_number = 1; part_number <= parts_count;  part_number++){
    erase_n_printf("Sending part %d of %d.",part_number,parts_count);
    strncpy (part, message+MAX_UCS2_LEN*(part_number-1),MAX_UCS2_LEN);
    printf (".");
    part [MAX_UCS2_LEN+1]=0;
    if (pdu_encode (PDU, phone, part,message_ID,parts_count,part_number))
        {erase_n_printf("Encoding failed\n"); return 1;} printf (".");

    //����� � ������ ������ ���� ���������� 2 ���������, "0" � SCA �� ���������
    PDUlen = (strlen(PDU))/2-1;
    sprintf (tmp,"AT+CMGS=%d",PDUlen);
    if (IssueCommand (port,tmp ,tmp2,0) || !strchr(tmp2,'>')) {
        erase_n_printf("Sending failed\n");
        IssueCommand (port,"\032",tmp2, 0);
        return 1;
        }
    printf (".");
    if (commitPDU (port, PDU))
        {erase_n_printf("Sending failed\n"); return 1;}

    }
    erase_n_printf("Succes!\n");
}



/******************************************************************************/
/*                     ������������������ ����������� ������                  */
/* -------------------------------------------------------------------------- */
/* ��� ������������� � ����� SCA / DA PDU. �� �������� ���� SCA               */
/* -------------------------------------------------------------------------- */
/* Arguments: PDUed  - dest. pointer for converted phone number in PDU format */
/*            Phone   - src. pointer for phone number to be converted         */
/* Returns:   0 if succeded                                                   */
/*            1 if failed                                                     */
/******************************************************************************/

int PhoneToPDU (char* PDUed, char* Phone){
 char ConvertedPhone [MAX_PHONE_LEN+2];
 char odd, even;
 int len;
 len = strlen (Phone);
 if (len > MAX_PHONE_LEN) 
     {return 1;}                         //������� ������� �����
 strcpy (ConvertedPhone,Phone);
 
 if (len % 2){ 
     strcat (ConvertedPhone,"F");        //�������� �� ������� ����� ����������  
     len++;}

 while (len>0){                   
     even = ConvertedPhone[--len];        //�����, ��� len ������
     odd =  ConvertedPhone[--len];
     if (!isxdigit (even))
         {return 1;}                      //������������ ������ � ������
     if (!isxdigit (odd))
         {return 1;}                      //������������ ������ � ������        
     ConvertedPhone [len] = even;
     ConvertedPhone [len+1] = odd;
     }   
 strcpy (PDUed,ConvertedPhone);
 return 0;
}  

/******************************************************************************/
/*                   �������������� ������ cp1251 -> UCS2PDU                  */
/* -------------------------------------------------------------------------- */
/* ������� ����� win1251  (0xC0 - 0xFF) ������������� � UCS ����������� 0x350.*/
/* ��� �� �������� ���� "�" (0xA8 -> 0x401) � "�" (0xB8 -> 0x451)             */
/* ������� ����� ������� (�� 7F, ��������) �������� ��� ���������.            */
/* ��� ���������� ������ ������� � ����� ���, ��� �� �������� ����������.     */
/* � ����� ���������, ��� ��� ���������, � ���� � ��� - �� ����� �� �������.  */
/* ��� ������� � UCS2 -  ������������                                         */
/* ����� �� ��������, ��� ��� HEX'� � PDU ������������ ���������.             */
/* ����� �������, ���� ������ ����� �������� � ������ PDU 4 �����             */
/* -------------------------------------------------------------------------- */
/* Arguments: UCS2PDU  - dest. pointer for converted text in PDU format       */
/*            Cp1251String   - src. pointer for cp1251 text to be converted   */
/* Returns:   0 if succeded                                                   */
/*            1 if failed                                                     */
/******************************************************************************/

int Convert_Cp1251_To_UCS2PDU (char* UCS2PDU , char* Cp1251String){
 char dest [MAX_UCS2_LEN*4+1]="";
 char extender [5]="";
 unsigned int new_symbol;
 int srclen=strlen ((char*)Cp1251String);
 if (srclen > MAX_UCS2_LEN)
    {return 1;}                             //������� ����� ����
 for (int i=0; i<srclen; i++){
    new_symbol = Cp1251String[i];           //��������

    if  ((unsigned char)Cp1251String[i]==0xA8)             
        {new_symbol = 0x401;}               //�
    if  ((unsigned char)Cp1251String[i]==0xB8)
        {new_symbol = 0x451;}               //�
    if  ((unsigned char)Cp1251String[i]>=0xC0)
        {new_symbol =(unsigned char)Cp1251String[i]+0x350;}//�-�
    sprintf (extender,"%04X", new_symbol);
    strcat  (dest,extender);
    }
 strcpy (UCS2PDU,dest);
 return 0;
}            

/******************************************************************************/
/*                      ������ SMS PDU (�� ��������, �.�. SMS-SUBMIT          */
/* -------------------------------------------------------------------------- */
/* |  SCA  |PDU-type| MR  |	  DA  |  PID |	 DCS |    VP   |  UDL  |    UD  | */        
/* -------------------------------------------------------------------------- */
/* |1-12B  |  1B 	| 1B  |2-12 B | 1B   | 1 B   |0,1 or 7B|  1 B  | 0-140 B| */
/* -------------------------------------------------------------------------- */
/* SCA - ����� (�����) SMS - ������                                           */
/* PDU - protocol description unit - ���� ������ ��������� - ������ �����     */
/* MR  - message reference - ����� ��������� � ��������                       */
/* DA  - ����� ����������                                                     */
/* PID - Protocol identifier - ������������� ���������                        */ 
/* DCS - Data coding scheme  - ����� �����������.                             */
/* VP  - Validity-Period - ������ ��������                                    */
/* UDL - User data length - ����� ��������� (������� ���� ������)             */  
/* UD  - User Data - ��������� (�� �����, ������� ���� ������)                */  
/*                                                                            */
/* ��� ���� ������������� ��, ��� ��� ����������������� (����� �������).      */
/* �� � ������� ��� ������������ � ���� ��������. �.�. ���� ���� �����        */
/* �������� 41H, �� ���������� ��� �������: 34H ("4") � 31H ("1").            */
/* ��� �� �������� ���������� �������, ��������� �����, � ��� ����������� BCD */
/* � ��� ���������� ����.                                                     */  
/*                                                                            */
/* ����:http://www.dreamfabric.com/sms/ (eng, the best)                       */
/*      http://www.ixbt.com/mobile/review/comp-sms.shtml (rus)                */
/*      http://imania.zp.ua/addinfo_pdu.php  (rus , code examples)            */
/*      http://ru.wikibooks.org/wiki/�������_��_�����������_��������_SMS (rus)*/          
/*      http://www.isms.ru/ - ��� �� ��� - ���������                          */
/*  �� � �������, ��������� ����� ��� ������:                                 */
/*      http://en.wikipedia.org/wiki/Concatenated_SMS (en)                    */ 
/* -------------------------------------------------------------------------- */
/* Arguments: PDU     - pointer to PDU string formmatted by the function      */
/*             phone   - string containing number to deliver message to       */
/*            message - string (cp1251) with the message text                 */
/*            CMSID   - unique number of multipart SMS - omit for std. SMS    */
/*            CSMSQ   - count of parts of multipart SMS - omit for std. SMS   */  
/*            CSMSN   - number of the particalar part - omit for std. SMS     */  
/* Returns:   0 if succeded                                                   */
/*            1 if failed                                                     */
/******************************************************************************/

int pdu_encode (char *PDU, char * phone, char * message,  unsigned char CSMSID,
                unsigned char CSMSQ, unsigned char CSMSN) 
{
 char tmp[2*MAX_PDU_LEN+1];
 char* tmps=tmp;
 char Phone4PDU[MAX_PHONE_LEN+2];               
 char Messg4PDU[MAX_UCS2_LEN*4+1];
 char extender[64];
 tmps[0]=0;

/******************************************************************************/
/*                      SCA - Service Center Address                          */
/* -------------------------------------------------------------------------- */
/* |     ����� ����        |         ��� ������     |        �����          | */
/* -------------------------------------------------------------------------- */
/* |         1B            |            0-1B       	|         0-6B          | */
/* -------------------------------------------------------------------------- */
/*  ����� �������� ����, ����������� ����� ������ SMSC + 1 ���� ����.         */
/*  ��� ������ 91H - �������������. ���� �� ������� - ��������� - �����       */
/*   �������� ��������� �� http://www.dreamfabric.com/sms/type_of_address.html*/
/*  ����� � ����������������� ��������� ������ ���� ����������� ��� �������   */
/*   "+" � ������ ���� ���� "����������". ������: ��� ������� ��� �� �����    */
/*   +70123456789 ������ ������ ���� ��������� "F" �� ������� ����� ����������*/
/*   �� ���� �������� ����� ������������� �: "70123456789F". ������ ��������� */
/*   ������ ���� ���� ������. �������: "0721436587F9".                        */
/*  ���� �������� ����� ���� = 0, ����� ������� ������ ����� ����� SMS-������ */
/*  �� ����� ��������. ���� �� ������� � ��� � �������. ��� ����� ������      */  
/*  �������� �� ������������ ������ SC � ������� ������������ �� �������� ���.*/
/******************************************************************************/

 sprintf (extender , "%02X" , 0x00);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          PDU-type                                          */
/* -------------------------------------------------------------------------- */
/* |    RP   |   UDHI   |   SRR    |	   VPF      |   RD   |	    MTI     | */
/* -------------------------------------------------------------------------- */
/* |    1b   |    1b 	|   1b     |        2b      |   1b   |      2b      | */
/* -------------------------------------------------------------------------- */
/* RP   - Reply path. ���������� ��� ��� �������� �����. TODO: �������� ���.  */
/* UDHI - User data header indicator. 1, ���� � ������ ���� ���������         */
/* SRR  - Status report request.    1 - ��������� ������ � ��������           */
/* VPF  - Validity Period Format.   0 - ���� ������� �������� (VP) � PDU ���  */
/*                                  1 - VP � "����������" ������� (7 ����)    */
/*                                  2 - VP � ������������� ������� (1 ����)   */
/*                                  3 - VP � ���������� �������    (7 ����)   */
/* RD	- Reject duplicates         1 - ��������� ��������� ���������� �����. */
/* MTI  - Message type indicator    1 ��� ������������� �������� (SMS-SUBMIT) */
/******************************************************************************/
 if (CSMSQ) sprintf (extender , "%02X" , 0x41);   
 else  sprintf (extender , "%02X" , 0x01);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          MR                                                */
/* -------------------------------------------------------------------------- */
/*  ���� ���������� MR � 0, �� �������� �������� ������� ��������������.      */
/*  � ����������� ��������� ������� ���� ������������������                   */
/******************************************************************************/

 sprintf (extender , "%02X" , 0x00);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          DA                                                */
/* -------------------------------------------------------------------------- */
/*  ������ ���������� ������� SCA                                             */
/******************************************************************************/
 sprintf (extender , "%02X" , strlen(phone)); //����� - ��� �����������. 
 strcat ( tmps , extender );            //���� ������� ���-�� ���� � ���. ������
 sprintf (extender , "%02X" , 0x91);    //91 - ����������� ������������� ������
 strcat ( tmps , extender );                  
 if (PhoneToPDU (Phone4PDU, phone))     //������������ �����
    {return 1;} 
 strcat ( tmps , Phone4PDU );

/******************************************************************************/
/*                          PID                                               */
/* -------------------------------------------------------------------------- */
/* �������� ������������� ������, ����� �������� ������� ������ ������        */
/* ������������ ��� ���������. 0 - ������� ���������. ���� �� �������.        */
/* �������� http://www.dreamfabric.com/sms/pid.html                           */
/******************************************************************************/

 sprintf (extender , "%02X" , 0x00);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          DCS                                               */
/* -------------------------------------------------------------------------- */
/* ����� ����������� ������ � ���� ������                                     */
/* bit                                                                        */
/*  7,6 - 00 ��� ����������� ����� (������ ��� ������������ Unicode)          */
/*  5 - 0 - ����� ����������������; 1 - ����� ��������������                  */
/*  4 - 0 - ������������ ���� 0 � 1; 1 - ��������� �� �������� ���� 0 � 1     */
/*  3,2 - 00 - ��������� �� ��������� (7bit) (ASCII)                          */
/*        01 - ��������� 8bit                                                 */
/*        10 - ��������� UCS2 (Unicode, 16 bit, ��������� ���������)          */
/*  1,0 - 00 - SMS Alert - ��������� ����������� ����� �� �����               */
/*        01 - ME specific                                                    */
/*        10 - SIM specific                                                   */
/*        11 - 	TE specific                                                   */
/* �������� http://www.dreamfabric.com/sms/pid.html                           */
/******************************************************************************/ 

 sprintf (extender , "%02X" , 0x08);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          VP                                                */
/* -------------------------------------------------------------------------- */
/* ��������� "���� �������� ���������". ����� ���� � �������������, ����������*/
/* � ���������� (enhaced) ��������. ��� VP �������� ������ VPF ���� PDU-Type. */
/* ���� VPF==0 �� ���� VP �����������. ���� ��� � �������.                    */
/******************************************************************************/


/******************************************************************************/
/*                          UDL                                               */
/* -------------------------------------------------------------------------- */
/* ����� ��������� ����������� � ������. �������������� ��� ������������      */
/*  ��������� UCS2 ����� ��������� � ��� ���� ������ ����� ������.            */  
/* ��� ������������� UDH - ��� �� 6 ���� ������                               */
/******************************************************************************/
 if (CSMSQ) sprintf (extender , "%02X" , 2*strlen (message)+6);
 else       sprintf (extender , "%02X" , 2*strlen (message));
 strcat ( tmps , extender );

/******************************************************************************/
/*                          UD                                                */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* ���� �� ����� ��������� ��� ��� ����� ���������������� ��� ���� ���������  */                                                            
/* ��� UDHI � ���� PDU-type � 1 � �������� ��������� ���������� ...           */
/******************************************************************************/

/******************************************************************************/
/*                UDH - ��������� ���������������� ������                     */
/* -------------------------------------------------------------------------- */
/* User Data Header -  6 ����                                                 */                                 
/* 0 UDHL - ����� ���������,  �� ���� 5 ����.                                 */
/* 1 IEI   - �������������� ���������. IEI=0 ��� ��������� SMS (08 ��� UCS2?) */
/* 2 ����� ��������� �� ����������� ����� IEI � ����� �����. ����� ������, 3. */
/* 3 ������������� ���������� ��������� - ���� ��� ���� ������ ���������      */
/* 4 ���������� ������ � ���������                                            */
/* 5 ���������� ����� ������ ����� � ��������� ���������                      */  
/* ��� �������� ��������� ��������� ����� ���������� ��������� ���������� ��  */
/* 6 �������, ���, � ������ � UCS2-������������ ���, �� 3 ����.               */
/******************************************************************************/
 
 if (CSMSQ) {
    sprintf (extender, "%02X%02X%02X%02X%02X%02X",5,0,3,CSMSID,CSMSQ,CSMSN); 
    strcat  ( tmps , extender );
 }
/******************************************************************************/
/*                        ���� ���������                                      */
/* -------------------------------------------------------------------------- */
/* ������������ ���� ��������� � UNICODE.                                     */
/******************************************************************************/    
 if (Convert_Cp1251_To_UCS2PDU (Messg4PDU , message)) return 1;
 strcat ( tmps , Messg4PDU );
 
 strcpy (PDU,tmps);
 return 0;
}        
         
        
    

