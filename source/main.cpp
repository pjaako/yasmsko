/******************************************************************************/
/*           ПРОГРАММА ДЛЯ ПОСЫЛКИ ДЛИННЫХ СМС ГРУППЕ АБОНЕНТОВ               */
/* -------------------------------------------------------------------------- */
/* Писано месяца марта года 2009 рабом Божьим Павлом Громовиковым             */
/* сыном Валерьевым. Адрес имеющим в сети вельми полезной, зело потешной и    */
/* и превеликой:        pavel@gromovikov.name.                                */  
/* Всякий обрядший сию машинопись да будет волен поступать с ней по своему    */
/* разумению да пользовать вольно, пусть и к прибытку, но оставивши запись    */
/* о первописателе.                                                           */  
/* Список телефонов должен называться "phones" и лежать в папке с екзешником  */
/* Текст сообщения должен называться "messages" и лежать в папке с екзешником */
/*   Уведомляю - рассылка рекламных смс без предварительного согласия         */
/*   получателя запрещена ст. 18 Федерального закона о рекламе!               */
/*                                                                            */
/******************************************************************************/

#include <cstdlib>
#include <iostream>
#include <windows.h>
 
#define MAX_PDU_LEN 176
#define MAX_PHONE_LEN 11
#define MIN_PHONE_LEN 11
#define MAX_UCS2_LEN 67  //на 3 символа меньше, чтобы влез UDH
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
/*                         ПРЕЖДЕ ЧЕМ МЫ НАЧНЕМ                               */
/* -------------------------------------------------------------------------- */
/*   Напоминаю - рассылка рекламных смс без предварительного согласия         */
/*   получателя запрещена ст. 18 Федерального закона о рекламе!               */
/*                                                                            */
/******************************************************************************/
//  
//
int main(int argc, char *argv[]) {
   
 char AnswBuf[COM_BUFF_LEN];
 char tmp2[64];
//строка должна быть в cp1251 и не больше MAX_UCS2_LEN
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
 
 //Перебираем список аргументов
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
 
 //ИЩЕМ ТЕЛЕФОН
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
    //ОТКРЫВАЕМ ФАЙЛ СООБЩЕНИЯ
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
    //ОТКРЫВАЕМ ФАЙЛ С ТЕЛЕФОНАМИ
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
 
 //НАСТРАИВАЕМ    
 if   (IssueCommand (port,"AT+CMGF=0",AnswBuf,1)) {
    printf ("Setting PDU mode failed. Exiting\n");
    goto END;
 }
   
 //ПОЛЕЗЛИ ПО СПИСКУ ТЕЛЕФОНОВ
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
 //ВЫХОД И УБОРКА ЗА СОБОЙ
 if (port) {
    // вернем порту его исходные параметры и закроем его, если открыт
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
/*                     Посылка AT-команды (любой) на тел.                     */
/* -------------------------------------------------------------------------- */
/* получаем нуль-терминированую строку команды и отправляем такую же          */
/* Дополняем необходимыми символами и вычисляем длину к отправке на месте     */
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
 DWORD db=0;                          //количество принятых/переданных байт
 char tmp2[COM_BUFF_LEN];
 char *tmp = tmp2;                         //буфер команды/ответа
 unsigned int CommandSize;                 //размер отправляемой команды

 if (strlen (Command)+4 > COM_BUFF_LEN)
    {return 1;}                            //длинновата команда так то...
 strcpy (tmp,Command);
 strcat (tmp, str_CR);
 CommandSize =  strlen (tmp);              //Формирование команды окончено


#ifdef SHOW_AT_DIALOG
/*Эта отладка позволяет нам посмотреть что конкретно мы говорим телефону*/
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

 WriteFile(port, tmp, CommandSize, &db, 0);//послали команду
 memset (tmp,0,sizeof (tmp2));              //почистили буфер
 db=0;
 ReadFile (port, tmp, sizeof(tmp2)-1, &db, 0);//прочитали ответ

#ifdef SHOW_AT_DIALOG
/*Эта отладка позволяет нам посмотреть что конкретно  телефон парит нам*/
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


 Sleep (100);//не гоним телефон слишком быстро.
 return 0;
}

/******************************************************************************/
/*                          Посылка  PDU на тел                               */
/* -------------------------------------------------------------------------- */
/*   Отличается тем, что вместо 0D надо посылать в конце ctl-Z  (1A),         */
/*   и тем что ожидание ответа может занять сравнительно большое время        */
/*   (время отсылки сообщения)                                                */
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

 char sequence [strlen(message)+2];//длина к отправке - PDU+CtrlZ+0
 sprintf (sequence,"%s%c",message,0x1a);
#ifdef SHOW_AT_DIALOG
 printf ("\n Sending %s as PDU\n",sequence);
#endif
 WriteFile(port, sequence, strlen(sequence), &dbc, 0);//послали сообщуху и Ctrl-Z
 //подождем ответа секунд 6
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
/*                          Поиск порта                                       */
/* -------------------------------------------------------------------------- */
/*        Тыкается в каждый доступный порт и посылает AT - команду            */
/*        Если получает ответ - считает что нашла телефон                     */  
/*        Если у кого то висит стандартный модем - сочувствую.                */
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
 //ПЕРЕБИРАЕМ ПОРТЫ 0-255
 for(int i = 0; i < 255; i++) {
    sprintf(tmp,"\\\\.\\COM%d ", i);
    //ПРОБУЕМ ОТКРЫТЬ
    port = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE,
            0,NULL,OPEN_EXISTING,0,NULL);
	if (port == INVALID_HANDLE_VALUE) continue;
    printf ("Port COM%d exists. Any phone here? ", i);
    //СОХРАНЯЕМ ИСХОДНЫЕ ПАРАМЕТРЫ ПОРТА
    GetCommTimeouts(port, &ComTimeOutsOld);
    memcpy (&CTO, &ComTimeOutsOld,sizeof(COMMTIMEOUTS));
    //ПЕРЕНАСТРАИВАЕМ ПОРТ ПОД СЕБЯ
    CTO.ReadIntervalTimeout=30;
    CTO.ReadTotalTimeoutMultiplier=0;
    CTO.ReadTotalTimeoutConstant=1000;
    SetCommTimeouts(port, &CTO);
    //ПОСЫЛАЕМ CTRL-Z (на случай если модем завис в ожидании PDU)
    //И ATE0 (отключаем эхо и проверяем откликнется ли модем)
    //ЕСЛИ ПРИШЕЛ ОТВЕТ ОК ИЛИ ХОТЯ БЫ ERROR - СЧИТАЕМ ЧТО НАШЛИ ТЕЛЕФОН 
    if  (!IssueCommand (port,"\032ATE0",AnswBuf,1)) 
        {phonefound = TRUE; break;}                         
    if (strstr (AnswBuf,"ERROR") && 
           (!IssueCommand (port,"\032ATE0",AnswBuf,1)))
        {phonefound = TRUE; break;} 
    printf ("No...\n");
    //ВОССТАНАВЛИВАЕМ ИСХОДНЫЕ ПАРАМЕТРЫ ПОРТА ЕСЛИ НЕ НАШЛИ ТЕЛЕФОНА
    SetCommTimeouts(port, &ComTimeOutsOld);
	CloseHandle(port); 
    }
    if (!phonefound) return 0;
    //ТЕЛЕФОН НАЙДЕН: РАДУЕМСЯ И ВЫЯСНЯЕМ ИМЯ - ОТЧЕСТВО
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
/*                        Отправка сообщения                                  */
/* -------------------------------------------------------------------------- */
/*          Дробит сообщение (если надо) и отсылает как части                 */
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
 char PDU [2*MAX_PDU_LEN+1];          //PDU для отправки через AT+CMGS
 unsigned char PDUlen=0;                       //длина PDU для AT+CMGS
 unsigned int message_len = strlen(message);

 printf ("Sending to %s\n",phone);
  //дадим пинка если сообщение слишком длинное
 if (message_len >(MAX_PARTS_COUNT*MAX_UCS2_LEN)){
    printf (" Message is too long \n",phone);
    return 1;
    }
 //сгенерируем "уникальный номер" для всех частей сообщения
 srand ( time(NULL) );
 message_ID = rand() % 0xFF + 1;
 //сколько всего частей?
 parts_count = (message_len/MAX_UCS2_LEN )+((message_len % MAX_UCS2_LEN)? 1:0);

 for (part_number = 1; part_number <= parts_count;  part_number++){
    erase_n_printf("Sending part %d of %d.",part_number,parts_count);
    strncpy (part, message+MAX_UCS2_LEN*(part_number-1),MAX_UCS2_LEN);
    printf (".");
    part [MAX_UCS2_LEN+1]=0;
    if (pdu_encode (PDU, phone, part,message_ID,parts_count,part_number))
        {erase_n_printf("Encoding failed\n"); return 1;} printf (".");

    //Длина в байтах кажный байт кодируется 2 символами, "0" в SCA не считается
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
/*                     Переформатирование телефонного номера                  */
/* -------------------------------------------------------------------------- */
/* Для использования в полях SCA / DA PDU. См описание поля SCA               */
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
     {return 1;}                         //слишком длинный номер
 strcpy (ConvertedPhone,Phone);
 
 if (len % 2){ 
     strcat (ConvertedPhone,"F");        //добиваем до четного числа полубайтов  
     len++;}

 while (len>0){                   
     even = ConvertedPhone[--len];        //знаем, что len четное
     odd =  ConvertedPhone[--len];
     if (!isxdigit (even))
         {return 1;}                      //неправильные значки в номере
     if (!isxdigit (odd))
         {return 1;}                      //неправильные значки в номере        
     ConvertedPhone [len] = even;
     ConvertedPhone [len+1] = odd;
     }   
 strcpy (PDUed,ConvertedPhone);
 return 0;
}  

/******************************************************************************/
/*                   Форматирование текста cp1251 -> UCS2PDU                  */
/* -------------------------------------------------------------------------- */
/* Русские буквы win1251  (0xC0 - 0xFF) преобразуются в UCS добавлением 0x350.*/
/* Это не касается букв "Ё" (0xA8 -> 0x401) и "ё" (0xB8 -> 0x451)             */
/* Верхняя часть таблицы (до 7F, латиница) остается без изменений.            */
/* Для облегчения задачи оставим в покое все, что не является кириллицей.     */
/* И будем надеяться, что это правильно, а если и нет - то нигде не вылезет.  */
/* Все символы в UCS2 -  двухбайтовые                                         */
/* Также не забываем, что все HEX'ы в PDU записываются символами.             */
/* Таким образом, один символ будет занимать в строке PDU 4 байта             */
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
    {return 1;}                             //слижком многа букф
 for (int i=0; i<srclen; i++){
    new_symbol = Cp1251String[i];           //Латиница

    if  ((unsigned char)Cp1251String[i]==0xA8)             
        {new_symbol = 0x401;}               //Ё
    if  ((unsigned char)Cp1251String[i]==0xB8)
        {new_symbol = 0x451;}               //ё
    if  ((unsigned char)Cp1251String[i]>=0xC0)
        {new_symbol =(unsigned char)Cp1251String[i]+0x350;}//А-я
    sprintf (extender,"%04X", new_symbol);
    strcat  (dest,extender);
    }
 strcpy (UCS2PDU,dest);
 return 0;
}            

/******************************************************************************/
/*                      Формат SMS PDU (на отправку, т.н. SMS-SUBMIT          */
/* -------------------------------------------------------------------------- */
/* |  SCA  |PDU-type| MR  |	  DA  |  PID |	 DCS |    VP   |  UDL  |    UD  | */        
/* -------------------------------------------------------------------------- */
/* |1-12B  |  1B 	| 1B  |2-12 B | 1B   | 1 B   |0,1 or 7B|  1 B  | 0-140 B| */
/* -------------------------------------------------------------------------- */
/* SCA - адрес (номер) SMS - центра                                           */
/* PDU - protocol description unit - поле данных протокола - важная хуета     */
/* MR  - message reference - номер сообщения в телефоне                       */
/* DA  - номер получателя                                                     */
/* PID - Protocol identifier - идентификатор протокола                        */ 
/* DCS - Data coding scheme  - схема кодирования.                             */
/* VP  - Validity-Period - период годности                                    */
/* UDL - User data length - длина сообщения (которое идет дальше)             */  
/* UD  - User Data - сообщение (то самое, которое идет дальше)                */  
/*                                                                            */
/* Все поля подразумевают то, что они шестнадцатиричные (кроме битовых).      */
/* Но в телефон они отправляются в виде символов. Т.е. если поле имеет        */
/* значение 41H, то передаются два символа: 34H ("4") и 31H ("1").            */
/* Это не касается телефонных номеров, временных полей, у них извращенный BCD */
/* С ним разберемся ниже.                                                     */  
/*                                                                            */
/* Инфа:http://www.dreamfabric.com/sms/ (eng, the best)                       */
/*      http://www.ixbt.com/mobile/review/comp-sms.shtml (rus)                */
/*      http://imania.zp.ua/addinfo_pdu.php  (rus , code examples)            */
/*      http://ru.wikibooks.org/wiki/Вопросы_по_программной_отправке_SMS (rus)*/          
/*      http://www.isms.ru/ - все по смс - рассылкам                          */
/*  Ну и конечно, википедия рулит как всегда:                                 */
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
/* |     Длина поля        |         Тип номера     |        Номер          | */
/* -------------------------------------------------------------------------- */
/* |         1B            |            0-1B       	|         0-6B          | */
/* -------------------------------------------------------------------------- */
/*  Длина содержит байт, указывающий длину номера SMSC + 1 байт типа.         */
/*  Тип номера 91H - международный. Пока не паримся - приспичит - более       */
/*   подробно посмотрим на http://www.dreamfabric.com/sms/type_of_address.html*/
/*  Номер в интернациональном стандарте должен быть представлен без символа   */
/*   "+" и каждая пара цифр "развернута". Пример: Для посылки СМС на номер    */
/*   +70123456789 запись должна быть пополнена "F" до четного числа полубайтов*/
/*   То есть исходный номер преобразуется в: "70123456789F". Теперь развернем */
/*   каждую пару цифр номера. Получим: "0721436587F9".                        */
/*  Если параметр длина поля = 0, тогда телефон должен взять номер SMS-центра */
/*  из своих настроек. Пока не паримся а так и сделаем. Тем более многие      */  
/*  телефоны не переваривают номера SC а требуют использовать из настроек тлф.*/
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
/* RP   - Reply path. Существует или нет обратный адрес. TODO: смотреть эфф.  */
/* UDHI - User data header indicator. 1, если в данных есть заголовок         */
/* SRR  - Status report request.    1 - требовать отчета о доставке           */
/* VPF  - Validity Period Format.   0 - поля периода годности (VP) в PDU нет  */
/*                                  1 - VP в "улучшенном" формате (7 байт)    */
/*                                  2 - VP в относительном формате (1 байт)   */
/*                                  3 - VP в абсолютном формате    (7 байт)   */
/* RD	- Reject duplicates         1 - отклонять повторные идентичные сообщ. */
/* MTI  - Message type indicator    1 для отправляемого собщения (SMS-SUBMIT) */
/******************************************************************************/
 if (CSMSQ) sprintf (extender , "%02X" , 0x41);   
 else  sprintf (extender , "%02X" , 0x01);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          MR                                                */
/* -------------------------------------------------------------------------- */
/*  Если установить MR в 0, то референс назначит телефон самостоятельно.      */
/*  С назначением референса вручную надо экспериментировать                   */
/******************************************************************************/

 sprintf (extender , "%02X" , 0x00);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          DA                                                */
/* -------------------------------------------------------------------------- */
/*  Формат аналогичен формату SCA                                             */
/******************************************************************************/
 sprintf (extender , "%02X" , strlen(phone)); //длина - тут разночтения. 
 strcat ( tmps , extender );            //пока возьмем кол-во цифр в исх. номере
 sprintf (extender , "%02X" , 0x91);    //91 - стандартный международный формат
 strcat ( tmps , extender );                  
 if (PhoneToPDU (Phone4PDU, phone))     //конвертируем номер
    {return 1;} 
 strcat ( tmps , Phone4PDU );

/******************************************************************************/
/*                          PID                                               */
/* -------------------------------------------------------------------------- */
/* Сообщает транспортному уровню, какой протокол высшего уровня должен        */
/* обрабатывать это сообщение. 0 - обычное сообщение. Пока не паримся.        */
/* Подробно http://www.dreamfabric.com/sms/pid.html                           */
/******************************************************************************/

 sprintf (extender , "%02X" , 0x00);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          DCS                                               */
/* -------------------------------------------------------------------------- */
/* Схема кодирования данных в поле данных                                     */
/* bit                                                                        */
/*  7,6 - 00 для стандартной схемы (только она поддерживает Unicode)          */
/*  5 - 0 - текст некомпрессирован; 1 - текст компрессирован                  */
/*  4 - 0 - игнорировать биты 0 и 1; 1 - принимать во внимание биты 0 и 1     */
/*  3,2 - 00 - кодировка по умолчанию (7bit) (ASCII)                          */
/*        01 - кодировка 8bit                                                 */
/*        10 - кодировка UCS2 (Unicode, 16 bit, поддержка кириллицы)          */
/*  1,0 - 00 - SMS Alert - сообщение выскакивает сразу на экран               */
/*        01 - ME specific                                                    */
/*        10 - SIM specific                                                   */
/*        11 - 	TE specific                                                   */
/* Подробно http://www.dreamfabric.com/sms/pid.html                           */
/******************************************************************************/ 

 sprintf (extender , "%02X" , 0x08);
 strcat ( tmps , extender );

/******************************************************************************/
/*                          VP                                                */
/* -------------------------------------------------------------------------- */
/* Указывает "срок годности сообщения". Может быть в относительном, абсолютном*/
/* и улучшенном (enhaced) форматах. Тип VP задается битами VPF поля PDU-Type. */
/* Если VPF==0 то поле VP отсутствует. Пока так и сделаем.                    */
/******************************************************************************/


/******************************************************************************/
/*                          UDL                                               */
/* -------------------------------------------------------------------------- */
/* Длина сообщения указывается в байтах. Соответственно для двухбайтовой      */
/*  кодировки UCS2 длина сообщения в два раза больше длины строки.            */  
/* При импользовании UDH - еще на 6 байт больше                               */
/******************************************************************************/
 if (CSMSQ) sprintf (extender , "%02X" , 2*strlen (message)+6);
 else       sprintf (extender , "%02X" , 2*strlen (message));
 strcat ( tmps , extender );

/******************************************************************************/
/*                          UD                                                */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* Если мы хотим отправить смс как часть многостраничного нам надо выставить  */                                                            
/* бит UDHI в поле PDU-type в 1 и снабдить сообщение заголовком ...           */
/******************************************************************************/

/******************************************************************************/
/*                UDH - заголовок пользовательских данных                     */
/* -------------------------------------------------------------------------- */
/* User Data Header -  6 байт                                                 */                                 
/* 0 UDHL - длина заголовка,  То есть 5 байт.                                 */
/* 1 IEI   - информационный индикатор. IEI=0 для составных SMS (08 для UCS2?) */
/* 2 длина заголовка за исключением байта IEI и этого байта. Проще говоря, 3. */
/* 3 идентификатор составного сообщения - одно для всех частей сообщения      */
/* 4 количество частей в сообщении                                            */
/* 5 порядковый номер данной части в составном сообщении                      */  
/* При отправке составных сосбщений длина отдельного сообщения уменьшится на  */
/* 6 октетов, или, в случае с UCS2-кодированным смс, на 3 симв.               */
/******************************************************************************/
 
 if (CSMSQ) {
    sprintf (extender, "%02X%02X%02X%02X%02X%02X",5,0,3,CSMSID,CSMSQ,CSMSN); 
    strcat  ( tmps , extender );
 }
/******************************************************************************/
/*                        Само сообщение                                      */
/* -------------------------------------------------------------------------- */
/* перекодируем наше сообщение в UNICODE.                                     */
/******************************************************************************/    
 if (Convert_Cp1251_To_UCS2PDU (Messg4PDU , message)) return 1;
 strcat ( tmps , Messg4PDU );
 
 strcpy (PDU,tmps);
 return 0;
}        
         
        
    

