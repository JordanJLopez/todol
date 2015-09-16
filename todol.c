//
//	todol: a simple, text-based to-do list
//
//	USAGE: todol <listfile> <action> [action parameters]
//
//	Actions: c=create, a=add, d=del, l=list, g=get, ch=change
//

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <sys/ioctl.h>

#define MAX_DATA 256
#define MAX_TASKS 20

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_CYAN_BOLD	   "\033[1;36m"

#define ANSI_FONT_BOLD	   "\x1b[1m"
#define ANSI_BACKGROUND_CYAN "\033[7;36m"

#define ENDHASH ANSI_BACKGROUND_CYAN " " ANSI_COLOR_RESET
#define ENDPIPE ANSI_BACKGROUND_CYAN " " ANSI_COLOR_RESET

struct Entry {
	int id;
	int set;
	int crossed;
	char task[MAX_DATA];
};

struct List {
	struct Entry tasks[MAX_TASKS];
};

struct Connection {
	FILE *file;
	struct List *ls;
};

void die(const char *message, struct Connection *conn);

// void die(const char *message)
// {
// 	if(errno) {
// 		perror(message);
// 	} else {
// 		printf("ERROR: %s\n", message);
// 	}


// 	exit(1);
// }


void Entry_print(struct Entry *entry)
{
	printf(ENDPIPE "%2d : %64s " ENDPIPE "\n", entry->id, entry->task);
}

//print x indicates that the task has been done, will be colored red
void Entry_printx(struct Entry *entry)
{
	printf(ANSI_COLOR_RED ANSI_FONT_BOLD " %2d:\t" ANSI_COLOR_RESET ANSI_COLOR_RED "%s\n" ANSI_COLOR_RESET, entry->id, entry->task);
}

void Entry_printf(struct Entry *entry)
{
	if(entry->crossed)
		Entry_printx(entry);
	else
		printf(ANSI_FONT_BOLD " %2d:\t" ANSI_COLOR_RESET "%s\n", entry->id, entry->task);
}



void List_load(struct Connection *conn)
{
	int rc = fread(conn->ls, sizeof(struct List), 1, conn->file);
	if(rc != 1)
		die("Failed to load list.", conn);
}

struct Connection *List_open(const char *filename, char mode)
{
	struct Connection *conn = malloc(sizeof(struct Connection));
	if(!conn)
		die("Memory error", conn);

	conn->ls = malloc(sizeof(struct List));
	if(!conn->ls)
		die("Memory error", conn);

	if(mode == 'c') {
		conn->file = fopen(filename, "w");
	} else {
		conn->file = fopen(filename, "r+");

		if(conn->file) {
			List_load(conn);
		}
	}

	if(!conn->file)
		die("Failed to open the file",conn);

	return conn;
}

void List_close(struct Connection *conn)
{
	if(conn) {
		if(conn->file)
			fclose(conn->file);
		if(conn->ls)
			free(conn->ls);
		free(conn);
	}
}

void List_write(struct Connection *conn)
{
	rewind(conn->file);

	int rc = fwrite(conn->ls, sizeof(struct List), 1, conn->file);
	if(rc != 1)
		die("Failed to write list.", conn);

	rc = fflush(conn->file);
	if(rc == -1)
		die("Cannot flush database.", conn);
}

void List_create(struct Connection *conn)
{
	int i = 0;

	for(i = 0; i < MAX_TASKS; i++)
	{
		struct Entry entry = {.id=i, .crossed = 0, .set=0};
		conn->ls->tasks[i] = entry;
	}
}

int List_add(struct Connection *conn, char* task)
{
	int i = 0;
	for(i = 0; i < MAX_TASKS; i++)
		if(!conn->ls->tasks[i].set) {
			strncpy(conn->ls->tasks[i].task, task, MAX_DATA);
			conn->ls->tasks[i].task[MAX_DATA - 1] = '\0';
			conn->ls->tasks[i].set = 1;
			return i;
		}

	die("List full: please remove an entry", conn);
	return -1;
}


int List_remove(struct Connection *conn, int id)
{
	//Checks if ID is within valid range
	if(id < 0 || id >= MAX_TASKS)
		die("ID out of range", conn);

	//Checks if ID is already empty
	if(!conn->ls->tasks[id].set)
		return 0;

	struct Entry empty = {.id = id, .crossed = 0, .set = 0};

	//Checks if ID is the greatest possible ID
	if(id == MAX_TASKS -1)
	{
		conn->ls->tasks[id] = empty;
		return 0;
	}

	//Checks if ID + 1 is empty, and if so, just remove ID
	if(!conn->ls->tasks[id+1].set)
	{
		conn->ls->tasks[id] = empty;
		return 0;
	}

	//Else, move data from ID + 1 to ID, then call to remove ID + 1;

	conn->ls->tasks[id] = conn->ls->tasks[id+1];
	return List_remove(conn, id+1);
}

int List_crossout(struct Connection *conn, int id)
{
	if(id < 0 || id >= MAX_TASKS)
		die("ID out of range", conn);
	if(!conn->ls->tasks[id].set)
		return 0;

	if(conn->ls->tasks[id].crossed)
		conn->ls->tasks[id].crossed = 0;
	else
		conn->ls->tasks[id].crossed = 1;

	return 1;
}

void List_list(struct Connection *conn)
{
	int i = 0;

	for(i = 0; i<MAX_TASKS; i++) {
		if(conn->ls->tasks[i].set)
			Entry_print(&conn->ls->tasks[i]);
	}
}

void List_get(struct Connection *conn, int id)
{
	if(id<0 || id>=MAX_TASKS)
		die("ID out of range", conn);
	if(conn->ls->tasks[id].set)
		Entry_print(&conn->ls->tasks[id]);
	else
		die("No task with given ID", conn);
}

void Todol_header(int columns)
{
	//
	//	Todol_header creates one of several header lines randomly for Todol,
	//

	//For MAXDATA = 64, header and footer length must be 71 + # + \n
	//r is a random number between 0 and 9
	if(columns < 38)
	{
		printf(ANSI_BACKGROUND_CYAN "%*s" ANSI_COLOR_RESET "\n",columns,"");
	}
	else
	{
		srand(time(NULL));
		int r = rand() % 10;
		printf("\n"ANSI_BACKGROUND_CYAN "%*s" ANSI_COLOR_RESET "\n",columns,"");
		switch(r)
		{
			case 0: // "# todol: You got this!" + " #" length: 24
				printf(ENDHASH ANSI_CYAN_BOLD " todol: You got this!" ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 24, "");
				break;
			case 1: // "# todol: I believe in you!" + " #" length:28
				printf(ENDHASH ANSI_CYAN_BOLD" todol: I believe in you!" ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 28, "");
				break;
			case 2: // "# todol: Just do it! -Shia LaBeouf" + " #" length:36
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Just do it! -Shia LaBeouf" ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 36, "");
				break;
			case 3: // "# todol: Make it a todone-l" + " #" length:29
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Make it a todone-l" ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 29, "");
				break;
			case 4: // "# todol: You're awesome." + " #" length:26
				printf(ENDHASH ANSI_CYAN_BOLD" todol: You're awesome." ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 26, "");
				break;
			case 5: // "# todol: You make it look easy." + " #" length:33
				printf(ENDHASH ANSI_CYAN_BOLD" todol: You make it look easy." ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 33, "");
				break;
			case 6: // "# todol: You're on fire today!" + " #" length:32
				printf(ENDHASH ANSI_CYAN_BOLD" todol: You're on fire today!" ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 32, "");
				break;
			case 7: // "# todol: Don't give up!" + " #" length:25
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Don't give up!" ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 25, "");
				break;
			case 8: // "# todol: Take it one step at a time." + " #" length:38
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Take it one step at a time." ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 38, "");
				break;
			case 9: // "# todol: Show this list who's boss!" + " #" length:37
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Show this list who's boss!" ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 37, "");
				break;
			default: // "# todol: Nothing is impossible!" + " #" length:33
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Nothing is impossible!" ANSI_COLOR_RESET "%*s " ENDHASH "\n",columns - 33, "");
				//Ironically, should be impossible to see this.
				break;
		}
	}

	printf(ANSI_BACKGROUND_CYAN "%*s" ANSI_COLOR_RESET "\n",columns,"");

}

void Todol_footer(int columns)
{
	if(columns > 10)
	{
		char* footertext = "h=help  ";
		printf(ANSI_BACKGROUND_CYAN "%*s%s" ANSI_COLOR_RESET "\n",columns - 8,"",footertext);

	}
	else
	{
		char filler[columns];
		printf(ANSI_BACKGROUND_CYAN "%*s " ANSI_COLOR_RESET "\n", columns, "");
	}
}

void Todol_emptylist()
{
	printf(ENDPIPE ANSI_CYAN_BOLD "%23sYou finished everything!%23s" ANSI_COLOR_RESET ENDPIPE "\n","","");
}

void Todol_emptylistf(int columns)
{
	if(columns < 26)
		printf("\n");
	else
	{
		int centerJustify = (columns - 24)/2;
		printf(ANSI_CYAN_BOLD "\n%*sYou finished everything!\n\n" ANSI_COLOR_RESET, centerJustify, "");
	}

}

void List_listf(struct Connection *conn)
{
	struct winsize sz;
	if(!ioctl(0, TIOCGWINSZ, &sz))
	{
		Todol_header(sz.ws_col);
		printf("\n");
		int i = 0;
		if(!conn->ls->tasks[0].set)
			Todol_emptylistf(sz.ws_col);
		else
			for(i = 0; i < MAX_TASKS; i++)
			{
				if(conn->ls->tasks[i].set)
					Entry_printf(&conn->ls->tasks[i]);
			}
		printf("\n");
		Todol_footer(sz.ws_col);
	}
	else
	{
		Todol_header(72);
		List_list(conn);
		Todol_footer(72);
	}
}

int List_clearcross(struct Connection *conn)
{
	int i = 0;
	for(i = MAX_TASKS - 1; i >= 0; i--)
	{
		if(!conn->ls->tasks[i].set)
			return 0;
		if(conn->ls->tasks[i].crossed == 1)
			List_remove(conn, i);
	}
	return 1;
}

void die(const char *message, struct Connection *conn)
{
	if(errno) {
		perror(message);
	} else {
		printf("ERROR: %s\n", message);
	}

	List_close(conn);
	exit(1);
}

void dieSimple(const char *message)
{
	if(errno) {
		perror(message);
	} else {
		printf("ERROR: %s\n", message);
	}
	exit(1);
}

int main(int argc, char *argv[])
{
	if(argc < 2)
		dieSimple("USAGE: todol <action> [action parameters]");

	//char* filename = argv[1];
	char action = argv[1][0];

	struct Connection *conn = List_open("todol.txt", action);

	int i;
	switch(action){
		case 'n':
			List_create(conn);
			List_write(conn);
			break;
		case 'a':
			printf("Enter new task: \n");
			char instr[MAX_DATA];
			fgets(instr, MAX_DATA-1, stdin);

			//Scrubs input of newline character
			char *pos;
			if((pos = strchr(instr, '\n')) != NULL)
				*pos = '\0';
			instr[MAX_DATA - 1] = '\0';
			printf("New ID: %d\n", List_add(conn, instr));
			List_write(conn);
			break;
		case 'x':
			if(argc < 3)
				die("Need an ID to crossout", conn);
			List_crossout(conn, atoi(argv[2]));
			List_write(conn);
			break;
		case 'r':
			if(argc < 3)
				die("Need an ID to remove", conn);
			List_remove(conn, atoi(argv[2]));
			List_write(conn);
			break;
		case 'l':
			List_listf(conn);
			break;
		case 'g':
			if(argc < 3)
				die("Need an ID to get", conn);
			List_get(conn, atoi(argv[2]));
		case 'p':
			for(i = MAX_TASKS -1; i>=0; i--)
				if(conn->ls->tasks[i].set)
				{
					printf("Removing ID: %d\n", i);
					List_remove(conn, i);
					List_write(conn);
					break;
				}
			break;
		case 'h':
			printf("h=help, n=new list, a=add, x=crossout or uncrossout, l-list, g=get, p=pop\n");
			break;
		default:
			die("Invalid action, only: h=help, n=new list, a=add, x=crossout or uncrossout, l=list, g=get, p=pop",conn);
	}

	List_close(conn);
	return 0;
}
