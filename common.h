#ifndef COMMON_H
#define COMMON_H

#include <config.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>

#include "ltrace.h"
#include "defs.h"
#include "dict.h"
#include "sysdep.h"
#include "debug.h"
#include "ltrace-elf.h"
#include "read_config_file.h"

#if defined HAVE_LIBIBERTY || defined HAVE_LIBSUPC__
# define USE_DEMANGLE
#endif

extern char * command;

extern int exiting;  /* =1 if we have to exit ASAP */

enum arg_type {
	ARGTYPE_UNKNOWN = -1,
	ARGTYPE_VOID,
	ARGTYPE_INT,
	ARGTYPE_UINT,
	ARGTYPE_LONG,
	ARGTYPE_ULONG,
	ARGTYPE_OCTAL,
	ARGTYPE_CHAR,
	ARGTYPE_SHORT,
	ARGTYPE_USHORT,
	ARGTYPE_FLOAT,		/* float value, may require index */
	ARGTYPE_DOUBLE,		/* double value, may require index */
	ARGTYPE_ADDR,
	ARGTYPE_FILE,
	ARGTYPE_FORMAT,		/* printf-like format */
	ARGTYPE_STRING,		/* NUL-terminated string */
	ARGTYPE_STRING_N,	/* String of known maxlen */
	ARGTYPE_ARRAY,		/* Series of values in memory */
	ARGTYPE_ENUM,		/* Enumeration */
	ARGTYPE_STRUCT,		/* Structure of values */
	ARGTYPE_POINTER,	/* Pointer to some other type */
	ARGTYPE_COUNT		/* number of ARGTYPE_* values */
};

typedef struct arg_type_info_t {
	enum arg_type type;
	union {
		/* ARGTYPE_ENUM */
		struct {
			size_t entries;
			char ** keys;
			int * values;
		} enum_info;

		/* ARGTYPE_ARRAY */
		struct {
			struct arg_type_info_t * elt_type;
			size_t elt_size;
			int len_spec;
		} array_info;

		/* ARGTYPE_STRING_N */
		struct {
			int size_spec;
		} string_n_info;

		/* ARGTYPE_STRUCT */
		struct {
			struct arg_type_info_t ** fields;	/* NULL-terminated */
			size_t * offset;
			size_t size;
		} struct_info;

		/* ARGTYPE_POINTER */
		struct {
			struct arg_type_info_t * info;
		} ptr_info;

		/* ARGTYPE_FLOAT */
		struct {
			size_t float_index;
		} float_info;

		/* ARGTYPE_DOUBLE */
		struct {
			size_t float_index;
		} double_info;
	} u;
} arg_type_info;

enum tof {
	LT_TOF_NONE = 0,
	LT_TOF_FUNCTION,	/* A real library function */
	LT_TOF_FUNCTIONR,	/* Return from a real library function */
	LT_TOF_SYSCALL,		/* A syscall */
	LT_TOF_SYSCALLR,	/* Return from a syscall */
	LT_TOF_STRUCT		/* Not a function; read args from struct */
};

typedef struct Function Function;
struct Function {
	const char * name;
	arg_type_info * return_info;
	int num_params;
	arg_type_info * arg_info[MAX_ARGS];
	int params_right;
	Function * next;
};

enum toplt {
	LS_TOPLT_NONE = 0,	/* PLT not used for this symbol. */
	LS_TOPLT_EXEC,		/* PLT for this symbol is executable. */
	LS_TOPLT_POINT		/* PLT for this symbol is a non-executable. */
};

extern Function * list_of_functions;
extern char *PLTs_initialized_by_here;

struct library_symbol {
	char * name;
	void * enter_addr;
	char needs_init;
	enum toplt plt_type;
	char is_weak;
	struct library_symbol * next;
};

struct opt_c_struct {
	int count;
	struct timeval tv;
};

#include "options.h"
#include "output.h"
#ifdef USE_DEMANGLE
#include "demangle.h"
#endif

extern Dict * dict_opt_c;

enum process_status {
	ps_invalid,	/* Failure.  */
	ps_stop,	/* Job-control stop.  */
	ps_tracing_stop,
	ps_sleeping,
	ps_zombie,
	ps_other,	/* Necessary other states can be added as needed.  */
};

/* Events  */
enum ecb_status {
	ecb_cont, /* The iteration should continue.  */
	ecb_yield, /* The iteration should stop, yielding this
		    * event.  */
	ecb_deque, /* Like ecb_stop, but the event should be removed
		    * from the queue.  */
};
extern Event * next_event(void);
extern Event * each_qd_event(enum ecb_status (* cb)(Event * event, void * data),
			     void * data);
extern void enque_event(Event * event);
extern void handle_event(Event * event);

extern pid_t execute_program(const char * command, char ** argv);
extern int display_arg(enum tof type, Process * proc, int arg_num, arg_type_info * info);
extern void disable_all_breakpoints(Process * proc);

extern void show_summary(void);
extern arg_type_info * lookup_prototype(enum arg_type at);

extern int do_init_elf(struct ltelf *lte, const char *filename);
extern void do_close_elf(struct ltelf *lte);
extern int in_load_libraries(const char *name, struct ltelf *lte, size_t count, GElf_Sym *sym);
extern struct library_symbol *library_symbols;
extern void add_library_symbol(GElf_Addr addr, const char *name,
		struct library_symbol **library_symbolspp,
		enum toplt type_of_plt, int is_weak);

extern struct library_symbol * clone_library_symbol(struct library_symbol * s);
extern void destroy_library_symbol(struct library_symbol * s);
extern void destroy_library_symbol_chain(struct library_symbol * chain);

struct breakpoint;

/* Arch-dependent stuff: */
extern char * pid2name(pid_t pid);
extern pid_t process_leader(pid_t pid);
extern int process_tasks(pid_t pid, pid_t **ret_tasks, size_t *ret_n);
extern int process_stopped(pid_t pid);
extern enum process_status process_status(pid_t pid);
extern void trace_set_options(Process * proc, pid_t pid);
extern void wait_for_proc(pid_t pid);
extern void trace_me(void);
extern int trace_pid(pid_t pid);
extern void untrace_pid(pid_t pid);
extern void get_arch_dep(Process * proc);
extern void * get_instruction_pointer(Process * proc);
extern void set_instruction_pointer(Process * proc, void * addr);
extern void * get_stack_pointer(Process * proc);
extern void * get_return_addr(Process * proc, void * stack_pointer);
extern void set_return_addr(Process * proc, void * addr);
extern void enable_breakpoint(Process * proc, struct breakpoint *sbp);
extern void disable_breakpoint(Process * proc, struct breakpoint *sbp);
extern int syscall_p(Process * proc, int status, int * sysnum);
extern void continue_process(pid_t pid);
extern void continue_after_signal(pid_t pid, int signum);
extern void continue_after_syscall(Process *proc, int sysnum, int ret_p);
extern void continue_after_breakpoint(Process * proc, struct breakpoint *sbp);
extern void continue_after_vfork(Process * proc);
extern long gimme_arg(enum tof type, Process * proc, int arg_num, arg_type_info * info);
extern void save_register_args(enum tof type, Process * proc);
extern int umovestr(Process * proc, void * addr, int len, void * laddr);
extern int umovelong (Process * proc, void * addr, long * result, arg_type_info * info);
extern size_t umovebytes (Process *proc, void * addr, void * laddr, size_t count);
extern int ffcheck(void * maddr);
extern void * sym2addr(Process *, struct library_symbol *);
extern int linkmap_init(Process *, struct ltelf *);
extern void arch_check_dbg(Process *proc);
extern int task_kill (pid_t pid, int sig);

/* Called when trace_me or primary trace_pid fail.  This may plug in
 * any platform-specific knowledge of why it could be so.  */
void trace_fail_warning(pid_t pid);

/* A pair of functions called to initiate a detachment request when
 * ltrace is about to exit.  Their job is to undo any effects that
 * tracing had and eventually detach process, perhaps by way of
 * installing a process handler.
 *
 * OS_LTRACE_EXITING_SIGHANDLER is called from a signal handler
 * context right after the signal was captured.  It returns 1 if the
 * request was handled or 0 if it wasn't.
 *
 * If the call to OS_LTRACE_EXITING_SIGHANDLER didn't handle the
 * request, OS_LTRACE_EXITING is called when the next event is
 * generated.  Therefore it's called in "safe" context, without
 * re-entrancy concerns, but it's only called after an even is
 * generated.  */
int os_ltrace_exiting_sighandler(void);
void os_ltrace_exiting(void);

extern struct ltelf main_lte;

#endif
