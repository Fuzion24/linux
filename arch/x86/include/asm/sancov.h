#ifndef _ASM_X86_SANCOV_H
#define _ASM_X86_SANCOV_H

#ifdef CONFIG_SANCOV

#define SANCOV_SYSCALL_ENTER			\
	pushq	%rax;				\
	pushq	%rcx;				\
	pushq	%rdx;				\
	pushq	%rdi;				\
	pushq	%rsi;				\
	pushq	%r8;				\
	pushq	%r9;				\
	pushq	%r10;				\
	pushq	%r11;				\
	call	sancov_syscall_enter;		\
	popq	%r11;				\
	popq	%r10;				\
	popq	%r9;				\
	popq	%r8;				\
	popq	%rsi;				\
	popq	%rdi;				\
	popq	%rdx;				\
	popq	%rcx;				\
	popq	%rax;				\
/**/

#define SANCOV_SYSCALL_EXIT			\
	call	sancov_syscall_exit;		\
/**/

#else /* ifdef CONFIG_SANCOV */

#define SANCOV_SYSCALL_ENTER
#define SANCOV_SYSCALL_EXIT

#endif /* ifdef CONFIG_SANCOV */
#endif /* ifndef _ASM_X86_SANCOV_H */
