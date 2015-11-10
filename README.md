# yasmsko
This program allows to send SMS messages specified as a command-line option or a file to a number or a list of numbers.

Numbers may be listed in "numbers.txt" file or defined in command-line options. Long SMSes are allowed - they will be divided to parts, which then will be assembled in receiving cellphone in one so-called "composite SMS". The program will automatically recode cp1251 text to UCS2, thus allowing to send SMS in russian.

This is a Windows program, using windows way of addressing COM ports.

Warning! This program sends a text message when invoked with default configuration file. Make sure to change "numbers.txt" prior to running if you want to avoid uncovering your phone number or charging for a text message.
Exported from code.google.com/p/yasmsko
