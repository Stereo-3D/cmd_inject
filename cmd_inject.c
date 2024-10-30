#define CMD_INJECT_VERSION "v0.4.3"
#define LICENSE "===[LICENSE.txt]===\n\
cmd_inject: Command injector for both Steam Windows and Steam Linux\n\
            plus some other launcher with editable launch command\n\
Original project link on Github: https://github.com/Stereo-3D/cmd_inject\n\
Special Thanks:\n\
  - Jose Negrete AKA BlueSkyDefender <UntouchableBlueSky@gmail.com>\n\
      for leting me extract and using list of App hash database from his awesome \"Overwatch.fxh\":\n\
        https://github.com/BlueSkyDefender/Depth3D/blob/master/Shaders/Overwatch.fxh\n\
Copyright © 2024 Tjandra Satria Gunawan\n\
  (Email: tjandra@yandex.com | Youtube: https://youtube.com/@TjandraSG | Discord: tjandra)\n\
License: GNU GENERAL PUBLIC LICENSE\n\
         Version 3, 29 June 2007\n\
         https://www.gnu.org/licenses/gpl-3.0.en.html\n\
List of terms inside GPL v3: ^for more detailed terms visit the link above^:\n\
  Permissions:\n\
    - Anyone can copy, modify, and distribute this software.\n\
    - You can use this software privately.\n\
    - You can use this software for commercial purposes.\n\
  Conditions:\n\
    - License and copyright notice: You have to include the license and copyright notice\n\
                                    with each and every distribution.\n\
    - State changes: If you modify it, you have to indicate changes made to the code.\n\
    - Same license: Any modifications of this code base MUST be distributed\n\
                    with the same license, GPLv3.\n\
    - Disclose source: Source code must be made available\n\
                       when the licensed material is distributed.\n\
  Limitations:\n\
    - Warranty: This software is provided without warranty.\n\
    - Liability: The software author or license can not be held liable for\n\
                 any damages inflicted by the software.\n"
#include<time.h>//give the prorgam ability to convert the date and detect timezone
#include<stdio.h>//give the program ability to read and write file
#include<stdlib.h>//give the program ability to re-allocate memory
#include<dirent.h>//give the program ability to scan game folder
#include<sys/time.h>//give the program ability to query system time and measure program speed
#include"hash.h"//using ReShade's app hash function and database extracted from Overwatch.fxh
typedef struct Dynamic_String{char*data;int length,alloc;}dstr;
int check_if_str_is_prefix_of_dstr(dstr*str,int str_index,char*keyword)
{
	while(*keyword!='\0'&&str_index<str->length)
		if(str->data[str_index++]!=*keyword++)return 0;
	return*keyword=='\0';
}
int check_for_special_program_keyword(dstr*str,int left_index,int right_index)
{
	//generating data backup before doing check operation
	char backup_char,left_backup,right_backup;
	if(str->data[left_index]==str->data[right_index]&&
		(str->data[left_index]=='\''||str->data[left_index]=='\"'))
			left_backup=right_backup=str->data[left_index];
	else if(right_index-left_index>2&&
		check_if_str_is_prefix_of_dstr(str,left_index,"\\\"")&&
		check_if_str_is_prefix_of_dstr(str,right_index-1,"\\\""))
	{
		left_backup=str->data[++left_index];
		right_backup=str->data[--right_index];
	}
	else
	{
		left_backup=str->data[--left_index];
		right_backup=str->data[++right_index];
	}
	//change the data format to make checking process easier
	str->data[left_index]=str->data[right_index]='\"';
	backup_char=str->data[right_index+1];
	str->data[right_index+1]='\0';
	str->data+=left_index;
	str->length-=left_index;
	//begin checking
	int satisfy=0,i,ret_code=0;
	if(is_both_string_equal(str->data,"\"waitforexitandrun\""))
	{
		ret_code=1;//proton's magic keyword detected
		goto restore_data_and_return;
	}
	for(i=right_index-left_index+1;i--;)if(str->data[i]=='\\'||str->data[i]=='/')break;
	if(i<0)i=0;
	if(check_if_str_is_prefix_of_dstr(str,++i,"proton")){i+=6;satisfy=1;}//proton binary
	else if(check_if_str_is_prefix_of_dstr(str,i,"wine")){i+=4;satisfy=1;}//wine binary
	if(!satisfy)
	{
		//linux shell binary (so many of them)! this list may not a complete list
		if(check_if_str_is_prefix_of_dstr(str,i,"bash")||
			check_if_str_is_prefix_of_dstr(str,i,"fish")||
			check_if_str_is_prefix_of_dstr(str,i,"tcsh")||
			check_if_str_is_prefix_of_dstr(str,i,"dash")){i+=4;satisfy=1;}
		else if(check_if_str_is_prefix_of_dstr(str,i,"csh")||
			check_if_str_is_prefix_of_dstr(str,i,"ksh")||
			check_if_str_is_prefix_of_dstr(str,i,"zsh")){i+=3;satisfy=1;}
		else if(check_if_str_is_prefix_of_dstr(str,i,"sh")){i+=2;satisfy=1;}
		if(satisfy&&str->data[i]=='\"')ret_code=4;//linux shell binary detected
		goto restore_data_and_return;
	}
	//suffix like 32 and 64 is also allowed
	if(check_if_str_is_prefix_of_dstr(str,i,"32")||
		check_if_str_is_prefix_of_dstr(str,i,"64"))i+=2;
	//in case if there is wine or proton exe version (unlikely)
	if(check_if_str_is_prefix_of_dstr(str,i,".exe"))i+=4;
	if(str->data[i]=='\"')ret_code=2;//wine or proton binary detected
	//end of check time to restore the data to original
	restore_data_and_return:;
	str->data-=left_index;
	str->length+=left_index;
	str->data[left_index]=left_backup;
	str->data[right_index]=right_backup;
	str->data[right_index+1]=backup_char;
	return ret_code;
}
dstr log_str;FILE*log_file;char*argv0;int skip_clock;
void append_char_to_dstr(dstr*str,char c,int escape_special)//and log file IO management
{
	if(str==&log_str&&log_file==NULL)
	{
		//because log file is an emergency it need to be prioritized! (have own safe code)
		int i,j;char*log_file_path;for(i=-1;argv0[++i];);
		log_file_path=(char*)calloc(i+128,sizeof(char));
		while(i--)if(argv0[i]=='\\'||argv0[i]=='/')break;
		for(j=i+1,i=-1;++i<j;)log_file_path[i]=argv0[i];
		sprintf(log_file_path+i,"cmd_inject.log");
		log_file=fopen(log_file_path,"w");
		if(log_file==NULL)
		{
			printf("[ERROR] cannot write to log file: \"%s\"! ",log_file_path);
			printf("Make sure you have write permission on that folder and make ");
			printf("sure you have enough disk space to write the file to the disk!\n");
			fflush(stdout);
		}
		free(log_file_path);
	}
	if(str->length==str->alloc)
		str->data=(char*)realloc(str->data,(str->alloc=str->alloc<<1|1)*sizeof(char));
	if((c=='\"'||c=='\\')&&escape_special)
	{
		str->data[str->length++]='\\';
		if(str==&log_str)fputc('\\',log_file);
		append_char_to_dstr(str,c,0);
		return;
	}
	str->data[str->length++]=c;
	if(str==&log_str)
	{
		if(c!='\n'&&!skip_clock)//print the timestamp in log file if non-blank line is detected
		{
			char dates[64];struct timeval now;skip_clock=1;
			gettimeofday(&now,NULL);time_t now_time=now.tv_sec;
			strftime(dates,sizeof(dates),"%a|%d-%b-%y|%Z|%H:%M:%S",localtime(&now_time));
			fprintf(log_file,"[%s.%.6ld] ",dates,now.tv_usec);
		}
		if(c!='\0')fputc(c,log_file);
		if(c=='\n'){skip_clock=0;fflush(log_file);}//force to write the log data to disk (clear cache)
		/* the above operation is slow (especially on non ssd drive)
		 * but this is necessary to capture all moments while the program alive
		 * to ensure the log data is saved to disk properly in case of program crash at random
		 */
		if(c=='\0'){fclose(log_file);log_file=NULL;}
	}
	return;
}
void append_string_to_dstr(dstr*str,char*string,char end_symbol,int write_quotes)
{
	if(write_quotes)append_char_to_dstr(str,'\"',0);
	while(*string)append_char_to_dstr(str,*string++,write_quotes);
	if(write_quotes)append_char_to_dstr(str,'\"',0);
	append_char_to_dstr(str,end_symbol,0);
	return;
}
dstr*argvy;int argiy,argly;char buffer[512],mindiff[512];//argvy is storing game's arguments
void replace_argvy(char*value)
{
	append_string_to_dstr(&log_str,"New auto-detected game exe argument:",' ',0);
	append_string_to_dstr(&log_str,value,'\n',1);argvy->length=0;
	append_string_to_dstr(argvy,value,'\0',1);
	return;
}
void append_game_argument(dstr*game_arg,int left,int right)
{
	sprintf(buffer,"  --> Game argument [%d]:",argiy);
	append_string_to_dstr(&log_str,buffer,' ',0);
	if(argly==argiy)argvy=(dstr*)realloc(argvy,(argly=argly<<1|1)*sizeof(dstr));
	int i;dstr*str;str=argvy+argiy++;str->data=NULL;str->length=str->alloc=0;
	append_char_to_dstr(str,'\"',0);
	for(i=left;++i<right;)append_char_to_dstr(str,game_arg->data[i],0);
	append_char_to_dstr(str,'\"',0);append_char_to_dstr(str,'\0',0);
	append_string_to_dstr(&log_str,str->data,'\n',0);
	return;
}
typedef struct FOLDER_OBJECT{DIR*dir;dstr paths;int depth;}fold;
fold*folders;int folder_alloc,folder_len;char folder_separator='/';
int depth_limit,depth_limit_work_dir=1,depth_limit_game_dir=4;//4 should be enough for most UE games
int append_folder(dstr*path,char*folder_name,int depth)
{
	if(folder_len==folder_alloc)
		folders=(fold*)realloc(folders,(folder_alloc=folder_alloc<<1|1)*sizeof(fold));
	int index=--path->length,ret_code=2;
	append_string_to_dstr(path,folder_name,folder_separator,0);//combine path and folder name
	append_char_to_dstr(path,'\0',0);
	//initialize folder data
	fold*fldr;fldr=folders+folder_len;
	fldr->depth=depth+1;fldr->paths.data=NULL;
	fldr->paths.alloc=fldr->paths.length=0;
	if((fldr->dir=opendir(path->data))!=NULL)//try opening folder: /path/foldername
	{
		if(depth<depth_limit)//add the new folder to the queue
		{
			append_string_to_dstr(&fldr->paths,path->data,'\0',0);
			++folder_len;ret_code=0;
		}
		else{closedir(fldr->dir);ret_code=1;}//depth limit reached
	}
	path->length=index;append_char_to_dstr(path,'\0',0);//restore path to original state
	return ret_code;
}
void scan_dir_for_exact_name(dstr*exe)
{
	append_string_to_dstr(&log_str,"\n===[Folder Scanner]===",'\n',0);
	int match=0,index=exe->length,i,folder_now,min_diff=999,depth;
	char c;DIR*dir;struct dirent*ent;dstr paths,fname;
	fname.length=fname.alloc=0;fname.data=NULL;//initialize data
	while(index--&&(c=exe->data[index])!='\\'&&c!='/');//find the separator between path and filename
	append_string_to_dstr(&fname,exe->data+index+1,'\0',0);//backup and copy filename
	if(!++index)goto try_opening_work_dir;//if no path data found then try opening work dir instead
	c=folder_separator=exe->data[exe->length=index-1];//get folder separator symbol (could be '\' or '/')
	append_char_to_dstr(exe,'\0',0);//remove filename (leaving only the game paths)
	depth_limit=depth_limit_game_dir;//set folder search limit
	if(append_folder(exe,"",0))//try opening folder from game args
	{
		append_string_to_dstr(&log_str,"[WARN] Cannot open folder:",' ',0);
		append_string_to_dstr(&log_str,exe->data,'\n',1);
		try_opening_work_dir:;
		exe->length=0;append_string_to_dstr(exe,".",'\0',0);
		depth_limit=depth_limit_work_dir;folder_separator='/';
		if(append_folder(exe,"",0))//try opening current working directory
		{
			//cannot open any folder :( bad os permission?
			append_string_to_dstr(&log_str,"[WARN] Cannot open any folder,",' ',0);
			append_string_to_dstr(&log_str,"folder scanner will be aborted!",'\n',0);
		}
	}
	for(folder_now=-1;++folder_now<folder_len;)//Breadth-first search without recursion :v
	{
		dir=folders[folder_now].dir;
		depth=folders[folder_now].depth;
		paths=folders[folder_now].paths;
		append_string_to_dstr(&log_str,"Scanning folder:",' ',0);
		append_string_to_dstr(&log_str,paths.data,'\n',1);
		while((ent=readdir(dir))!=NULL)//walk through the current folder
		{
			if(is_both_string_equal(ent->d_name,"."))continue;//avoid going to this folder
			if(is_both_string_equal(ent->d_name,".."))continue;//avoid going to parent folder
			if((i=append_folder(&paths,ent->d_name,depth))<2)//trying to open the next folder
			{
				append_string_to_dstr(&log_str,"  --> Folder",' ',0);
				append_string_to_dstr(&log_str,ent->d_name,' ',1);
				if(i)append_string_to_dstr(&log_str,"is skipped because depth limit reached!",'\n',0);
				else append_string_to_dstr(&log_str,"is added to queue to be scanned later!",'\n',0);
				continue;
			}
			append_string_to_dstr(&log_str,"  --> Scanning file:",' ',0);
			append_string_to_dstr(&log_str,ent->d_name,'\n',1);
			if(check_hash(ent->d_name))//check match on Overwatch.fxh extracted database
			{
				append_string_to_dstr(&log_str,"      - Found hash match:",' ',0);
				append_string_to_dstr(&log_str,ent->d_name,'\n',1);
				if(!match++)
				{
					for(i=-1;ent->d_name[++i]!='\0';)buffer[i]=ent->d_name[i];
					buffer[i]='\0';
				}
				else append_string_to_dstr(&log_str,"      - [WARN] Multiple hash matches found!",'\n',0);
			}
			if((i=compare_case_insensitive(ent->d_name,fname.data))>=0)//check case insensitive match
			{
				append_string_to_dstr(&log_str,"      - Found case insensitive match:",' ',0);
				append_string_to_dstr(&log_str,ent->d_name,'\n',1);
				if(i<min_diff)
				{
					for(min_diff=i,i=-1;ent->d_name[++i]!='\0';)mindiff[i]=ent->d_name[i];
					mindiff[i]='\0';
				}
				else append_string_to_dstr(&log_str,"      - Rejecting the value because it's worse!",'\n',0);
			}
		}
		append_string_to_dstr(&log_str,"Closing folder:",' ',0);
		append_string_to_dstr(&log_str,paths.data,'\n',1);
		closedir(dir);free(paths.data);
		if(match)
		{
			append_string_to_dstr(&log_str,"Aborting search because hash match has been found!",'\n',0);
			break;//found the target exe, no need to check further
		}
	}
	while(++folder_now<folder_len)//closing the remaining opened folder
	{
		append_string_to_dstr(&log_str,"Closing folder:",' ',0);
		append_string_to_dstr(&log_str,folders[folder_now].paths.data,'\n',1);
		closedir(folders[folder_now].dir);free(folders[folder_now].paths.data);
	}
	exe->length=index?index-1:0;//restore exe length
	if(index)append_char_to_dstr(exe,c,0);//restore exe folder separator
	append_string_to_dstr(exe,fname.data,'\0',0);//restore exe filename
	if(0<min_diff&&min_diff<512)replace_argvy(mindiff);//case insensitive match found
	if(match==1)replace_argvy(buffer);//unique application hashes found
	if(fname.data)free(fname.data);//clean temporary filename variable
	if(folders)free(folders);//clean folder stack
	return;
}
dstr*argvx,launch_command,extra_command,game_exe;//argvx is storing this program's arguments
int arg_index,arg_index_inside_quotes,game_index,argix,arglx;
void convert_critical_argument_to_windows_format()//and copy arguments to corresponding var
{
	//Initializing the converter
	append_string_to_dstr(&log_str,"\n===[Argument Converter]===",'\n',0);
	char c=launch_command.data[arg_index],terminate_symbol=' ';
	int i=0,j,left=0,converting=1,escape=0;
	if(c=='\"'||c=='\''){terminate_symbol=c;i=1;}
	else if(arg_index_inside_quotes&&c=='\\'&&
		launch_command.data[arg_index+1]=='\"'){terminate_symbol='\"';i=2;}
	if(launch_command.data[arg_index+i]!='.'&&launch_command.data[arg_index+i]!='/')
	{
		append_string_to_dstr(&log_str,"No need to convert game argument!",'\n',0);
		converting=0;
	}
	else append_string_to_dstr(&log_str,"Converting game argument...",'\n',0);
	dstr game_arg;
	game_arg.data=NULL;
	game_arg.length=game_arg.alloc=0;
	//copying main game argument (and converting it to windows format if required)
	append_char_to_dstr(&game_arg,'\"',0);
	if(converting)
	{
		if(launch_command.data[arg_index+i]=='/')append_string_to_dstr(&game_arg,"Z",':',0);
		if(check_if_str_is_prefix_of_dstr(&launch_command,arg_index+i,"./"))
			{append_string_to_dstr(&game_exe,".",'/',0);i+=2;}
	}
	while((c=launch_command.data[arg_index+i++])!=terminate_symbol)
	{
		if(arg_index_inside_quotes&&!(escape&1)&&c=='\"')break;
		if(escape&1&&(c=='\"'||c=='\\'))game_exe.data[game_exe.length-1]=c;
		else append_char_to_dstr(&game_exe,c,0);
		if(c=='/'&&converting)append_char_to_dstr(&game_arg,'\\',1);
		else append_char_to_dstr(&game_arg,c,0);
		escape=c=='\\'?escape+1:0;
		if((escape&2)&&(c=launch_command.data[arg_index+i])==terminate_symbol)
		{
			escape=0;
			if(c==' '){game_arg.length-=2;--game_exe.length;}
			if(c!='\''){append_char_to_dstr(&game_arg,c,0);append_char_to_dstr(&game_exe,c,0);++i;}
		}
	}
	//finalizing the data for main game argument
	if(arg_index_inside_quotes&&(escape&1)&&c=='\"')game_arg.data[game_arg.length-1]=c;
	else append_char_to_dstr(&game_arg,'\"',0);
	append_string_to_dstr(&log_str,"Current game argument:",' ',0);
	append_char_to_dstr(&game_exe,'\0',0);
	append_string_to_dstr(&log_str,game_exe.data,'\n',1);
	if(converting)
	{
		append_char_to_dstr(&game_arg,'\0',0);
		append_string_to_dstr(&log_str,"New game argument:",' ',0);
		append_string_to_dstr(&log_str,game_arg.data,'\n',0);
		--game_arg.length;
	}
	append_string_to_dstr(&log_str,"Full game arguments:",'\n',0);
	for(j=game_arg.length;j--;)if(game_arg.data[j]=='\\'||game_arg.data[j]=='/')break;
	append_game_argument(&game_arg,j<0?0:j,game_arg.length-1);
	append_game_argument(&game_arg,0,game_arg.length-1);
	if(!arg_index_inside_quotes)
		for(j=game_index;++j<argix;)
			append_game_argument(argvx+j,0,argvx[j].length-2);
	//copy the remaining argument for game argument
	for(j=0;(c=launch_command.data[arg_index+i++])!='\0';)
	{
		if(!arg_index_inside_quotes)
		{
			append_char_to_dstr(&game_arg,c,0);
			continue;
		}
		if(!(escape&1)&&c=='\"')break;
		if(!j)
		{
			if(c==' '){escape=0;continue;}
			j=1;terminate_symbol=' ';
			if(c=='\\'&&launch_command.data[arg_index+i]=='\"')
				{++i;++escape;c=terminate_symbol='\"';}
			else if(c=='\'')terminate_symbol='\'';
			append_string_to_dstr(&game_arg," ",'\"',0);
			left=game_arg.length-1;
			if(terminate_symbol==' ')append_char_to_dstr(&game_arg,c,0);
		}
		else
		{
			if(c==terminate_symbol)
			{
				if(escape&2)//the terminate symbol is escaped
				{
					if(c==' ')game_arg.length-=2;//no escape needed for spaces inside quotes
					if(c=='\'')j=0;//symbol (') cannot be escaped!
				}
				else{j=0;if((escape&1)&&c=='\"')--game_arg.length;}
			}
			append_char_to_dstr(&game_arg,j?c:'\"',0);
			if(!j)append_game_argument(&game_arg,left,game_arg.length-1);
		}
		escape=c=='\\'?escape+1:0;
	}
	if(j)
	{
		append_char_to_dstr(&game_arg,j?c:'\"',0);
		append_game_argument(&game_arg,left,game_arg.length-1);
		if(terminate_symbol!=' ')
		{
			append_string_to_dstr(&log_str,"[WARN] Unexpexted end of string!",' ',0);
			append_string_to_dstr(&log_str,"Cannot find enclose symbol (",terminate_symbol,0);
			append_string_to_dstr(&log_str,") please check the command input!",'\n',0);
		}
	}
	//copy the remaining argument outside of linux shell if exist
	for(;c!='\0';c=launch_command.data[arg_index+i++])append_char_to_dstr(&extra_command,c,0);
	//finalizing the data for remaining variables
	append_char_to_dstr(&extra_command,'\0',0);
	free(game_arg.data);append_string_to_dstr(&log_str,"Done processing game argument!",'\n',0);
	return;
}
int steam_index,nested_shell_detected,launch_mode;
void append_argument(char*value)//also parse and identify each arguments on the fly
{
	dstr*str;int index,i,left,right,end,write_arg_index=0,escape;char c,terminate_symbol;
	sprintf(buffer,"Argument[%d]:",argix);
	append_string_to_dstr(&log_str,buffer,' ',0);
	if(arglx==argix)argvx=(dstr*)realloc(argvx,(arglx=arglx<<1|1)*sizeof(dstr));
	str=&argvx[index=argix++];
	str->data=NULL;
	str->length=str->alloc=0;
	append_string_to_dstr(str,value,'\0',1);
	append_string_to_dstr(&log_str,value,'\n',1);
	if(steam_index)//the remaining command after "--"
	{
		left=launch_command.length;
		append_string_to_dstr(&launch_command,value,' ',1);
		if(!nested_shell_detected)//not inside child linux shell
		{
			//do the argument check
			i=check_for_special_program_keyword(str,0,str->length-2);
			if(i&1)//proton magic keyword detected
			{
				arg_index=launch_command.length;game_index=argix;
				append_string_to_dstr(&log_str,"\n===[Game Argument]===",'\n',0);
			}
			if(i&2)//wine or proton executable detected
			{
				arg_index=launch_command.length;game_index=argix;
				append_string_to_dstr(&log_str,"^wine or proton arg above^",'\n',0);
			}
			if(i&4)//linux shell detected
			{
				nested_shell_detected=1;
				append_string_to_dstr(&log_str,"^bash shell fork above^",'\n',0);
			}
			return;
		}
		for(end=launch_command.length-2;left<end;)//scan the nested command (for linux shell)
		{
			if(launch_command.data[++left]<=' ')continue;
			if(write_arg_index){arg_index=left;write_arg_index=0;arg_index_inside_quotes=1;}
			//getting a token (sub argument) inside linux shell
			if(check_if_str_is_prefix_of_dstr(&launch_command,left,"\\\""))
				{terminate_symbol='\"';right=left+1;}
			else if(launch_command.data[left]=='\''){terminate_symbol='\'';right=left;}
			else{terminate_symbol=' ';right=left;}
			escape=launch_command.data[right]=='\\';
			while((c=launch_command.data[++right])!=terminate_symbol)
			{
				if(right>=end)
				{
					if(terminate_symbol!=' ')
					{
						append_string_to_dstr(&log_str,"[WARN] Unexpected end of stri",'n',0);
						append_string_to_dstr(&log_str,"g when parsing sub argument!",'\n',0);
					}
					right=end-1;break;
				}
				escape=c=='\\'?escape+1:0;
				if((escape&2)&&launch_command.data[right+1]==terminate_symbol)
				{
					if(terminate_symbol=='\'')
					{
						append_string_to_dstr(&log_str,"[WARN] symbol (\')",' ',0);
						append_string_to_dstr(&log_str,"cannot be escaped!",' ',0);
					}
					else++right;
				}
			}
			if(launch_command.data[right]==' '&&terminate_symbol==' ')--right;
			//print the sub argument to the log
			append_string_to_dstr(&log_str,"  --> sub argument:",' ',0);
			c=launch_command.data[right+1];
			launch_command.data[right+1]='\0';
			append_string_to_dstr(&log_str,launch_command.data+left,'\n',0);
			launch_command.data[right+1]=c;
			//do the sub argument check
			i=check_for_special_program_keyword(&launch_command,left,right);
			if(i&1)//proton magic keyword detected
			{
				write_arg_index=1;//the arg_index cannot be determined yet
				append_string_to_dstr(&log_str,"^proton magic keyword above^",'\n',0);
			}
			if(i&2)//wine or proton executable detected
			{
				write_arg_index=1;//the arg_index cannot be determined yet
				append_string_to_dstr(&log_str,"^wine or proton arg above^",'\n',0);
			}
			if(i&4)//If some launcher has this in practice, that launcher is very evil! :(
				append_string_to_dstr(&log_str,"[WARN] ^bash shell on bash shell^",'\n',0);
			left=right+1;
		}
		if(write_arg_index)//unable to properly parse linux shell argument (bad input?)
		{
			append_string_to_dstr(&log_str,"[WARN] No game argument found!",' ',0);
			append_string_to_dstr(&log_str,"But it should be there,",' ',0);
			append_string_to_dstr(&log_str,"discarding it anyway!",'\n',0);
		}
	}
	else if(is_both_string_equal(str->data,"\"--\"")||
		is_both_string_equal(str->data,"\"==\"")||
		is_both_string_equal(str->data,"\"~~\""))
	{
		if(is_both_string_equal(str->data,"\"--\""))launch_mode=3;//launch game and injectors
		if(is_both_string_equal(str->data,"\"==\""))launch_mode=2;//launch injector only
		if(is_both_string_equal(str->data,"\"~~\""))launch_mode=1;//no launch but generate log
		steam_index=index;game_index=argix;
		arg_index=launch_command.length;
		append_string_to_dstr(&log_str,"\n===[Steam Argument]===",'\n',0);
	}
	return;
}
void write_argument_to_file(FILE*f,int index)//write arg received by this program to file
{
	if(index>=steam_index)return;//for safety reasons, any arguments after "--" is not allowed
	fprintf(f,"%s",argvx[index].data);
	return;
}
void write_remaining_arguments_to_file(FILE*f,int from_index)
{
	for(;from_index<steam_index;)
	{
		write_argument_to_file(f,from_index);
		if(++from_index<steam_index)fputc(' ',f);
	}
	return;
}
void write_game_argument_to_file(FILE*f,int index)
{
	if(index>=argiy)return;
	fprintf(f,"%s",argvy[index].data);
	return;
}
void write_remaining_game_arguments_to_file(FILE*f,int from_index)
{
	for(;from_index<argiy;)
	{
		write_game_argument_to_file(f,from_index);
		if(++from_index<argiy)fputc(' ',f);
	}
	return;
}
/* Begin code for custom file I/O management (until "End of custom file I/O code" comment)
 * This is needed because using fopen directly will access the file from random directory:
 *  it could be launcer directory, steam directory, game directory or even injector directory
 * This custom I/O let the program to access the file under injector directory:
 *  the folder where the cmd_inject executable is located
 */
int injector_path_index;dstr injector_path;
void init_injector_path()//find the path of cmd_inject and save it
{
	append_string_to_dstr(&log_str,"\n===[injector path]===",'\n',0);
	append_string_to_dstr(&log_str,"Detecting injector path...",'\n',0);
	int i,j;
	for(j=argvx->length-1;j--;)
		if((argvx->data[j]=='\\'&&argvx->data[j+1]!='\"')||argvx->data[j]=='/')
			break;
	for(i=-1;i++<j;)append_char_to_dstr(&injector_path,argvx->data[i],0);
	if(!injector_path.data)append_char_to_dstr(&injector_path,'\"',0);
	injector_path_index=injector_path.length;
	append_char_to_dstr(&injector_path,'\0',0);
	append_string_to_dstr(&log_str,"injector folder location:",' ',0);
	append_string_to_dstr(&log_str,injector_path.data,'\"',0);
	append_string_to_dstr(&log_str,"!",'\n',0);
	return;
}
void create_full_file_injector_path(char*filename)//construct full path of filename
{
	injector_path.length=injector_path_index;
	while(*filename)append_char_to_dstr(&injector_path,*filename++,0);
	append_char_to_dstr(&injector_path,'\"',0);
	append_char_to_dstr(&injector_path,'\0',0);
	return;
}
FILE* open_file_on_injector_path(char*filename,char*mode)//open on the same path as cmd_inject
{
	FILE*f;
	create_full_file_injector_path(filename);
	injector_path.data[injector_path.length-2]='\0';
	f=fopen(injector_path.data+1,mode);
	injector_path.data[injector_path.length-2]='\"';
	if(f==NULL&&*mode!='r')
	{
		append_string_to_dstr(&log_str,"[ERROR] Cannot write to",' ',0);
		append_string_to_dstr(&log_str,injector_path.data,'!',0);
		append_string_to_dstr(&log_str," Make sure you have write permission on",' ',0);
		append_string_to_dstr(&log_str,"that folder and make sure you have enough",' ',0);
		append_string_to_dstr(&log_str,"disk space to write the file to the disk!",'\n',0);
	}
	return f;
}
//End of custom file I/O code
void load_app_list_extra(char*list_name)
{
	append_string_to_dstr(&log_str,"\n===[App List Loader]===",'\n',0);
	FILE*f;int cur;
	append_string_to_dstr(&log_str,"Opening app list file:",' ',0);
	append_string_to_dstr(&log_str,list_name,'\n',1);
	f=open_file_on_injector_path(list_name,"r");
	if(f==NULL)
	{
		append_string_to_dstr(&log_str,"App list file not found, generating new one!",'\n',0);
		generate_app_list_conf:;//generate the list
		f=open_file_on_injector_path(list_name,"w");
		fprintf(f,"#cmd inject version: %s\n",CMD_INJECT_VERSION);
		fprintf(f,"#source code: https://github.com/Stereo-3D/cmd_inject\n");
		fprintf(f,"#note: you can list your custom game apps here to be auto detected!\n");
		fprintf(f,"#      don't forget to remove the first line if you want to prevent'\n");
		fprintf(f,"#      this file overwritten by the program (keep changes permanent)'\n");
		fprintf(f,"#format: each line is a name of program in with .exe format exstension\n");
		fprintf(f,"#        you can put \'#\' at the begining of line to comment it\n");
		fprintf(f,"#There is 817+ apps list already stored inside this program!\n");
		fprintf(f,"#What you want to add here is most likely duplicates ;)\n");
		//fprintf(f,"GenshinImpact.exe\n");//add duplicates for testing purposes
		fprintf(f,"r5apex_dx12.exe");//no new line at the end of file for testing purposes
		fclose(f);
		f=open_file_on_injector_path(list_name,"r");
	}
	else
	{
		append_string_to_dstr(&log_str,"App list file found!",'\n',0);
		cur=fscanf(f,"%20c",buffer);buffer[20]='\0';//cheack if the file is autogenerated
		if(is_both_string_equal(buffer,"#cmd inject version:"))
		{
			append_string_to_dstr(&log_str,"App list file seems to be auto generated!",' ',0);
			append_string_to_dstr(&log_str,"Replacing it with new one...",'\n',0);
			fclose(f);goto generate_app_list_conf;
		}
	}
	dstr exe;exe.data=NULL;exe.alloc=0;hash_init();
	for(fseek(f,0,SEEK_SET);(cur=fgetc(f))!=EOF;)
	{
		if(cur=='\r'||cur=='\n')continue;
		if((char)cur=='#')
		{
			while((cur=fgetc(f))!=EOF&&(char)cur!='\r'&&(char)cur!='\n');//skip commented line
			continue;
		}
		exe.length=0;
		do append_char_to_dstr(&exe,(char)cur,0);
		while((cur=fgetc(f))!=EOF&&(char)cur!='\r'&&(char)cur!='\n');
		append_char_to_dstr(&exe,'\0',0);
		append_string_to_dstr(&log_str,"Adding",' ',0);
		append_string_to_dstr(&log_str,exe.data,' ',1);
		append_string_to_dstr(&log_str,"to the list...",'\n',0);
		if(hash_app_add(exe.data))
			append_string_to_dstr(&log_str,"[WARN] application is not in .exe exstension!",'\n',0);
	}
	if(exe.data)free(exe.data);
	sprintf(buffer,"Duplicate hashes found in app list: %d",hash_finalize());
	append_string_to_dstr(&log_str,buffer,'\n',0);fclose(f);
	return;
}
//Begin of Dictionary code to remember what exe from conf have been added to config.bat
typedef struct Dict{struct Dict*child[256];int exist;}dict;
dict*root;
int dict_check_item(char*keyword)
{
	dict*dct;
	for(dct=root;dct!=NULL&&*keyword!='\0';)dct=dct->child[(unsigned char)*keyword++];
	if(dct!=NULL&&dct->exist)return 1;
	return 0;
}
void dict_add_item(char*keyword)
{
	if(root==NULL)root=(dict*)calloc(1,sizeof(dict));
	unsigned char c;dict*dct;
	for(dct=root;(c=*keyword++)!=(unsigned char)'\0';dct=dct->child[c])
		if(dct->child[c]==NULL)dct->child[c]=(dict*)calloc(1,sizeof(dict));
	dct->exist=1;
	return;
}
void dict_destroy(dict*dct)
{
	int i;
	if(dct==NULL)return;
	for(i=-1;++i<256;)if(dct->child[i]!=NULL)dict_destroy(dct->child[i]);
	free(dct);
	return;
}
//End of dictionary code
void check_an_injector(FILE*f,char*injector_name,dstr*arg)//check any progam not just injector
{
	FILE*ff;int i,inside_quotes=0,escape;char c;
	ff=open_file_on_injector_path(injector_name,"r");
	append_string_to_dstr(&log_str,"  -->",' ',0);
	if(ff!=NULL)
	{
		fclose(ff);
		dict_add_item(injector_name);
		fprintf(f,"start \"\" \"%s\"",injector_name);
		for(i=-1;++i<arg->length;)//write the corresponding program arguments to config.bat
		{
			if(inside_quotes)
			{
				escape=(c=arg->data[i])=='\\'?escape+1:0;fputc(c,f);
				if(c=='\"'&&!(escape&1))inside_quotes=0;
				continue;
			}
			if((c=arg->data[i])=='\"'){inside_quotes=1;escape=0;fprintf(f," \"");continue;}
			if(c!='-'&&(c<'0'||'9'<c))continue;
			fprintf(f," %%");
			if(c=='-'){fputc('-',f);++i;}
			for(c='?',--i;++i<arg->length&&'0'<=(c=arg->data[i])&&c<='9';)fputc(c,f);
			if(c=='*')fputc(c,f);
		}
		fputc('\n',f);
		append_string_to_dstr(&log_str,"found",' ',0);
		append_string_to_dstr(&log_str,injector_name,' ',1);
		append_string_to_dstr(&log_str,"on injector path!",'\n',0);
	}
	else
	{
		append_string_to_dstr(&log_str,injector_name,' ',1);
		append_string_to_dstr(&log_str,"not found!",'\n',0);
	}
	return;
}
void load_injector_list(FILE*f,char*config_name)//can parse & auto generate the injector list
{
	FILE*ff;dstr arg,exe;
	int cur,max_arg_positive,max_arg_negative,neg,value,inside_quotes,escape;
	ff=open_file_on_injector_path(config_name,"r");
	append_string_to_dstr(&log_str,"Opening config file:",' ',0);
	append_string_to_dstr(&log_str,config_name,'\n',1);
	if(ff==NULL)
	{
		append_string_to_dstr(&log_str,"Config file not found, generating new one!",'\n',0);
		generate_injector_list_conf:;//generate the list
		ff=open_file_on_injector_path(config_name,"w");
		fprintf(ff,"#cmd inject version: %s\n",CMD_INJECT_VERSION);
		fprintf(ff,"#source code: https://github.com/Stereo-3D/cmd_inject\n");
		fprintf(ff,"#note: you can list your custom injector here to be auto detected!\n");
		fprintf(ff,"#      don't forget to remove the first line if you want to prevent'\n");
		fprintf(ff,"#      this file overwritten by the program (keep changes permanent)'\n");
		fprintf(ff,"#format: \"<injector exe>\" <arg_idx_a> <arg_idx_b> ... <arg_idx_n>\n");
		fprintf(ff,"#        don't forget to put quote your program name when editing!'\n");
		fprintf(ff,"#arg_idx: is either a string or an integer and can be negative\n");
		fprintf(ff,"#  if arg_idx is enclosed with quotes that means it's a constant\n");
		fprintf(ff,"#    string and the value will be copied exactly to the injector args\n");
		fprintf(ff,"#  if arg_idx is positivie then it access this program argument\n");
		fprintf(ff,"#  if arg_idx is negative then it access game argument: start from -1\n");
		fprintf(ff,"#  if arg_idx is equal to zero (0) that contain this program name \n");
		fprintf(ff,"#    and it's fullpath which is useless for the injector arguments'\n");
		fprintf(ff,"#  if arg_idx is equal to negative zero (-0) that contain the exe\n");
		fprintf(ff,"#    name of the game (without the path, just filename of game exe)\n");
		fprintf(ff,"#  if you put * after arg_idx then it will be replaced by remaining\n");
		fprintf(ff,"#    game or program argument starting from the given index until end\n");
		fprintf(ff,"#remember you could also edit \"config.bat\" to whatever you like\n");
		fprintf(ff,"#don't forget to remove the first commented line on \"config.bat\"!\n");
		fprintf(ff,"#so that your changes made to \"config.bat\" is kept permanent!\n");
		fprintf(ff,"#in other word to prevent \"config.bat\" overwritten by the program\n");
		//Nefarius Injector: https://github.com/nefarius/Injector
		fprintf(ff,"\"Injector.exe\" \"--process-name\" 1 \"--inject\" 2*\n");
		fprintf(ff,"\"Injector.exe\" \"--process-name\" -0 \"--inject\" 1*\n");
		//ReShade Injector: https://github.com/crosire/reshade
		fprintf(ff,"\"inject64.exe\" 1\n\"inject32.exe\" 1\n");
		fprintf(ff,"\"inject64.exe\" -0\n\"inject32.exe\" -0\n");
		//3DMigoto Loader: https://github.com/bo3b/3Dmigoto
		fprintf(ff,"\"3DMigoto Loader.exe\"\n");
		//Genshin FPS Unlocker: https://github.com/34736384/genshin-fps-unlock
		fprintf(ff,"\"unlockfps_nc.exe\"\n");
		fprintf(ff,"\"unlockfps_nc_signed.exe\"\n");
		//Another FPS Unlocker: (git link redacted)
		fprintf(ff,"\"fpsunlock.exe\" 1 2\n");
		fprintf(ff,"\"fpsunlock.exe\" 1\n");
		//jadeite loader-autopatcher: (git link redacted)
		fprintf(ff,"\"jadeite.exe\" 1 2\n");
		fprintf(ff,"\"jadeite.exe\" 1 \"--\"\n");
		//harmonic_loader: (git link redacted)
		fprintf(ff,"\"harmonic_loader.exe\" 1 2 3\n");
		fprintf(ff,"\"harmonic_loader.exe\" 1 2 \"--\"\n");
		fclose(ff);
		ff=open_file_on_injector_path(config_name,"r");
	}
	else
	{
		append_string_to_dstr(&log_str,"Config file found!",'\n',0);
		cur=fscanf(ff,"%20c",buffer);buffer[20]='\0';//cheack if the file is autogenerated
		if(is_both_string_equal(buffer,"#cmd inject version:"))
		{
			append_string_to_dstr(&log_str,"Config file seems to be auto generated!",' ',0);
			append_string_to_dstr(&log_str,"Replacing it with new one...",'\n',0);
			fclose(ff);goto generate_injector_list_conf;
		}
	}
	arg.data=NULL;arg.length=arg.alloc=0;
	exe.data=NULL;exe.length=exe.alloc=0;
	append_string_to_dstr(&log_str,"Detecting injectors:",'\n',0);
	for(fseek(ff,0,SEEK_SET),cur=fgetc(ff);cur!=EOF;)//parsing the entire lists
	{
		if((char)cur=='#')while((cur=fgetc(ff))!=EOF&&(char)cur!='\r'&&(char)cur!='\n');
		else if((char)cur=='\"')//valid line detected
		{
			exe.length=arg.length=escape=0;
			while((cur=fgetc(ff))!=EOF&&((char)cur!='\"'||escape&1)&&(char)cur!='\r'&&(char)cur!='\n')
			{
				append_char_to_dstr(&exe,(char)cur,0);//copy the exe name
				escape=cur=='\\'?escape+1:0;
			}
			append_char_to_dstr(&exe,'\0',0);
			if(cur=='\"')
			{
				max_arg_positive=max_arg_negative=inside_quotes=0;
				while((cur=fgetc(ff))!=EOF&&(char)cur!='\r'&&(char)cur!='\n')//parse the args
				{
					if(inside_quotes)
					{
						escape=cur=='\\'?escape+1:0;
						if(cur=='\"'&&!(escape&1))inside_quotes=0;
						append_char_to_dstr(&arg,cur,0);
						if(!inside_quotes)append_char_to_dstr(&arg,',',0);
						continue;
					}
					if(cur=='\"')
					{
						inside_quotes=1;
						append_char_to_dstr(&arg,cur,escape=0);
						continue;
					}
					if((char)cur!='-'&&((char)cur<'0'||'9'<(char)cur))continue;
					if((neg=cur=='-')){append_char_to_dstr(&arg,'-',0);cur=fgetc(ff);}
					for(value=0;'0'<=(char)cur&&(char)cur<='9';cur=fgetc(ff))
					{
						value=10*value+(cur&15);
						append_char_to_dstr(&arg,(char)cur,0);//copy the arg digit
					}
					if(neg&&value>max_arg_negative)max_arg_negative=value;
					if(!neg&&value>max_arg_positive)max_arg_positive=value;
					if(cur=='*')append_char_to_dstr(&arg,'*',0);
					append_char_to_dstr(&arg,',',0);//separate the arg (separator symbol: ',')
					if(cur==EOF||(char)cur=='\r'||(char)cur=='\n')break;
				}
				//finalize arg data
				if(inside_quotes)append_string_to_dstr(&arg,escape&1?"\\\"":"\"",',',0);
				if(!arg.length)*arg.data='\0';
				else arg.data[arg.length-1]='\0';
				if(max_arg_positive>=steam_index||max_arg_negative>=argiy)
				{
					append_string_to_dstr(&log_str,"  --> Detection of",' ',0);
					append_string_to_dstr(&log_str,exe.data,' ',1);
					append_string_to_dstr(&log_str,"is skipped because\n       ",' ',0);
					sprintf(buffer,"%s arguments",max_arg_negative<argiy?"program":"game");
					append_string_to_dstr(&log_str,buffer,' ',0);
					append_string_to_dstr(&log_str,"needed ",'(',0);
					sprintf(buffer,"%d) is exceeding given",
							max_arg_negative<argiy?max_arg_positive:max_arg_negative);
					append_string_to_dstr(&log_str,buffer,' ',0);
					sprintf(buffer,"%s arguments ",max_arg_negative<argiy?"program":"game");
					append_string_to_dstr(&log_str,buffer,'(',0);
					sprintf(buffer,"%d)!",max_arg_negative<argiy?steam_index-1:argiy-1);
					append_string_to_dstr(&log_str,buffer,'\n',0);
				}
				else if(dict_check_item(exe.data))
				{
					append_string_to_dstr(&log_str,"  --> [WARN] detection of",' ',0);
					append_string_to_dstr(&log_str,exe.data,' ',1);
					append_string_to_dstr(&log_str,"is skipped because\n       ",' ',0);
					append_string_to_dstr(&log_str,"the program has been",' ',0);
					append_string_to_dstr(&log_str,"added to \"config.bat\"!",'\n',0);
				}
				else check_an_injector(f,exe.data,&arg);
				//print all args to log file (useful for debuging)
				if(inside_quotes)
				{
					append_string_to_dstr(&log_str,"      [WARN] Unterminated quotes",' ',0);
					append_string_to_dstr(&log_str,"for arg constant! (unexpected end",' ',0);
					append_string_to_dstr(&log_str," of line)!",'\n',0);
				}
				append_string_to_dstr(&log_str,"      Full argument index",' ',0);
				append_string_to_dstr(&log_str,"list for that program: ",'[',0);
				append_string_to_dstr(&log_str,arg.data,']',0);
				append_char_to_dstr(&log_str,'\n',0);
			}
			else
			{
				append_string_to_dstr(&log_str,"  --> [WARN] Unterminated quotes for",' ',0);
				append_string_to_dstr(&log_str,"exe name! (unexpected end of line)!",'\n',0);
			}
		}
		while((cur=fgetc(ff))!=EOF)if(cur=='#'||cur=='\"')break;//#for comment, " for progname
	}
	append_string_to_dstr(&log_str,"Done detecting all injectors in the list!",'\n',0);
	if(arg.data)free(arg.data);
	if(exe.data)free(exe.data);
	dict_destroy(root);root=NULL;fclose(ff);
	return;
}
FILE* generate_bat_config(char*bat_filename)//return FILE pointer to generated config.bat
{
	/* Note that "config.bat" is not meant to execute directly because it only contain partial
	 *  bat command that will be processed further to create "launcher.bat"!
	 * It will not support receiving argument because the argument variables is different than
	 *  the standard bat script, the argument variables will be replaced by the exact argument
	 *  this program receive when generating "launcher.bat"!
	*/
	FILE*f,*ff;char*bfr;
	f=open_file_on_injector_path(bat_filename,"w");
	fprintf(f,"rem ove this line to make this config permanent! ");
	fprintf(f,"cmd_inject version: %s\n",CMD_INJECT_VERSION);
	fprintf(f,"rem ind that cmd_inject source code is available on github: ");
	fprintf(f,"https://github.com/Stereo-3D/cmd_inject\n");
	fprintf(f,"pushd %%~dp0\n");//bat command to change the directory to where config.bat is
	fprintf(f,"rem ember to put your injetor and its arguments between pushd and popd\n");
	if(launch_mode&2)//if launch mode enabled, need to generate & show the license to end user
	{
		ff=open_file_on_injector_path("LICENSE.txt","r");
		if(ff==NULL)//check if first time run
		{
			generate_license_txt:;//regenerate license
			append_string_to_dstr(&log_str,"Generating new \"LICENSE.txt\"...",'\n',0);
			ff=open_file_on_injector_path("LICENSE.txt","w");
			fprintf(ff,"cmd_inject version: %s\n",CMD_INJECT_VERSION);
			fprintf(ff,"%s",LICENSE);fclose(ff);
			fprintf(f,"start \"\" \"notepad.exe\" \"LICENSE.txt\"\n");//show the license
		}
		else
		{
			sprintf(buffer,"cmd_inject version: %s\n",CMD_INJECT_VERSION);
			for(bfr=buffer-1;*++bfr!='\0';)if((char)fgetc(ff)!=*bfr)//check validity of LICENSE.txt
			{
				append_string_to_dstr(&log_str,"\"LICENSE.txt\" is outdated!",'\n',0);
				fclose(ff);goto generate_license_txt;//LICENSE.txt is outdated, updating
			}
			for(bfr=LICENSE-1;*++bfr!='\0';)if((char)fgetc(ff)!=*bfr)//check validity more thoroughly
			{
				append_string_to_dstr(&log_str,"\"LICENSE.txt\" is changed and need to be fixed!",'\n',0);
				fclose(ff);goto generate_license_txt;//LICENSE.txt is changed, fixing
			}
			if(fgetc(ff)!=EOF)
			{
				append_string_to_dstr(&log_str,"\"LICENSE.txt\" has extra data and need to be fix!",'\n',0);
				fclose(ff);goto generate_license_txt;//It is forbiden to add more data on LICENSE.txt!
			}
			fclose(ff);//LICENSE.txt is valid and no need to show it again
			append_string_to_dstr(&log_str,"\"LICENSE.txt\" is valid!",'\n',0);
		}
	}
	load_injector_list(f,"injecttor_list.conf");
	fprintf(f,"popd\n");//bat command to go back to previous directory you are in before pushd
	if(launch_mode&1)fprintf(f,"start \"\" %%-1*\n");
	fclose(f);
	return open_file_on_injector_path(bat_filename,"r");
}
void load_cmd_inject_config(char*ini_filename)
{
	FILE*f;int cur,i;
	f=open_file_on_injector_path(ini_filename,"r");
	append_string_to_dstr(&log_str,"\n===[cmd_inject config]===",'\n',0);
	append_string_to_dstr(&log_str,"Opening configuration file:",' ',0);
	append_string_to_dstr(&log_str,ini_filename,'\n',1);
	if(f==NULL)
	{
		append_string_to_dstr(&log_str,"Configuration file not found! creating new one...",'\n',0);
		create_ini_file:;
		f=open_file_on_injector_path(ini_filename,"w");
		fprintf(f,";cmd inject version: %s\n",CMD_INJECT_VERSION);
		fprintf(f,";remove the first line if you want to change the variable value!\n");
		fprintf(f,";depth limit work dir: the recursion depth to scan folder on work dir\n");
		fprintf(f,";depth limit game dir: the recursion depth to scan folder on game dir\n");
		fprintf(f,"[cmd_inject]\n");
		fprintf(f,"depth_limit_work_dir = 1\n");
		fprintf(f,"depth_limit_game_dir = 4\n");
		fclose(f);
		f=open_file_on_injector_path(ini_filename,"r");
	}
	else
	{
		append_string_to_dstr(&log_str,"Configuration file found!",'\n',0);
		cur=fscanf(f,"%20c",buffer);buffer[20]='\0';//cheack if the file is autogenerated
		if(is_both_string_equal(buffer,";cmd inject version:"))
		{
			append_string_to_dstr(&log_str,"Configuration file seems to be auto generated!",' ',0);
			append_string_to_dstr(&log_str,"Replacing it with new one...",'\n',0);
			fclose(f);goto create_ini_file;
		}
	}
	for(fseek(f,0,SEEK_SET);(cur=fscanf(f,"%s",buffer))!=EOF;)
	{
		if(is_both_string_equal(buffer,"depth_limit_work_dir"))
		{
			while((cur=fgetc(f))!=EOF&&(char)cur!='='&&(char)cur!='\r'&&(char)cur!='\n');
			if((char)cur=='=')
			{
				cur=fscanf(f,"%d",&i);
				depth_limit_work_dir=i;
				sprintf(buffer,"depth_limit_work_dir = %d",i);
				append_string_to_dstr(&log_str,buffer,'\n',0);
			}
		}
		else if(is_both_string_equal(buffer,"depth_limit_game_dir"))
		{
			while((cur=fgetc(f))!=EOF&&(char)cur!='='&&(char)cur!='\r'&&(char)cur!='\n');
			if((char)cur=='=')
			{
				cur=fscanf(f,"%d",&i);
				depth_limit_game_dir=i;
				sprintf(buffer,"depth_limit_game_dir = %d",i);
				append_string_to_dstr(&log_str,buffer,'\n',0);
			}
		}
		if(cur!=EOF)while((cur=fgetc(f))!=EOF&&(char)cur!='\r'&&(char)cur!='\n');
		if(cur==EOF)break;
	}
	fclose(f);
	return;
}
int main(int argc,char**argv)
{
	//receiving argumnents parameter and initializing the program
	int i,j,ret=0;FILE*f,*ff;argv0=argv[0];
	sprintf(buffer,"cmd_inject version: %s\nSource code:",CMD_INJECT_VERSION);
	append_string_to_dstr(&log_str,buffer,' ',0);
	append_string_to_dstr(&log_str,"https://github.com/Stereo-3D/cmd_inject",'\n',0);
	append_string_to_dstr(&log_str,"\n===[Injector Argument]===",'\n',0);
	for(i=-1;++i<argc;)append_argument(argv[i]);
	append_string_to_dstr(&log_str,"\n===[Launch Mode]===",'\n',0);
	sprintf(buffer,"Launch mode: \"%s\"!",launch_mode<3?launch_mode<2?!launch_mode?
		"Error":"No Launch":"Injector Only":"All Programs Will be Launched");
	append_string_to_dstr(&log_str,buffer,'\n',0);
	if(launch_command.data==NULL)
	{
		append_string_to_dstr(&launch_command,"\"",'\"',0);
		append_string_to_dstr(&log_str,"[WARN] launch command is empty! Make sure",' ',0);
		append_string_to_dstr(&log_str,"you have put \"--\" before %command%!",'\n',0);
	}
	if(arg_index==launch_command.length)append_string_to_dstr(&launch_command,"\"",'\"',0);
	convert_critical_argument_to_windows_format();
	init_injector_path();
	load_cmd_inject_config("cmd_inject.ini");
	load_app_list_extra("game_app_list.conf");
	scan_dir_for_exact_name(&game_exe);
	//creating or parsing "config.bat"
	f=open_file_on_injector_path("config.bat","r");
	if(f==NULL)
	{
		append_string_to_dstr(&log_str,"\n===[Config Bat Log]===",'\n',0);
		f=generate_bat_config("config.bat");
	}
	else
	{
		j=fscanf(f,"%8c",buffer);buffer[8]='\0';//cheack if the file is autogenerated
		if(is_both_string_equal(buffer,"rem ove "))
		{
			append_string_to_dstr(&log_str,"\n===[Config Bat Log]===",'\n',0);
			append_string_to_dstr(&log_str,"Overwriting \"config.bat\" file!",'\n',0);
			fclose(f);f=generate_bat_config("config.bat");
		}
	}
	//generating "launcher.bat"
	ff=open_file_on_injector_path("launcher.bat","w");
	for(fseek(f,0,SEEK_SET);(i=fgetc(f))!=EOF;fputc(i,ff))if('%'==(char)i)
	{
		if((char)(j=fgetc(f))=='-'||('0'<=(char)j&&(char)j<='9'))
		{
			*buffer='+';buffer[1]='?';
			if(j=='-'){*buffer='-';j=fgetc(f);}
			for(i=j,j=0;'0'<=(char)i&&(char)i<='9';i=fgetc(f))j=10*j+(i&15);
			if((char)i=='*'){buffer[1]='*';i=fgetc(f);}
			if((char)i<=' ')
			{
				if(*buffer=='-'&&buffer[1]=='*')write_remaining_game_arguments_to_file(ff,j);
				if(*buffer=='+'&&buffer[1]=='*')write_remaining_arguments_to_file(ff,j);
				if(*buffer=='-'&&buffer[1]=='?')write_game_argument_to_file(ff,j);
				if(*buffer=='+'&&buffer[1]=='?')write_argument_to_file(ff,j);
			}
			else fprintf(ff,"%%%s%d%s",*buffer=='-'?"-":"",j,buffer[1]=='*'?"*":"");
		}
		else{fputc('%',ff);i=j;}
		if(i==EOF)break;
	}
	fclose(f);fclose(ff);
	//attempting to inject launcher.bat to launch command
	launch_command.length=arg_index;
	create_full_file_injector_path("launcher.bat");
	if(arg_index_inside_quotes)
		injector_path.data[injector_path.length-2]=injector_path.data[0]='\'';
	append_string_to_dstr(&launch_command,injector_path.data,'?',0);
	--launch_command.length;//remove the extra '?' symbol from launch_command
	append_string_to_dstr(&launch_command,extra_command.data,'\0',0);
	append_string_to_dstr(&log_str,"\n===[Launch Command]===",'\n',0);
	append_string_to_dstr(&log_str,launch_command.data,'\0',0);
	//cleaning up and freeing memory
	for(i=-1;++i<argix;)free(argvx[i].data);
	free(argvx);free(extra_command.data);free(injector_path.data);
	free(log_str.data);free(game_exe.data);free(hashes);
	if(launch_mode&2)ret=system(launch_command.data);//launching injected launch command
	free(launch_command.data);
	return ret;
}
