/*
 * void cswitch(struct kctx *from, struct kctx *to);
 */
	.globl cswitch
	.globl hswitch
cswitch:
	movl	4(%esp), %eax	/* %eax <- from */
	movl	8(%esp), %edx	/* %edx <- to */

	/* save the old kernel context */
	movl	0(%esp), %ecx
	movl	%ecx, 20(%eax)
	movl	%ebp, 16(%eax)
	movl	%ebx, 12(%eax)
	movl	%esi, 8(%eax)
	movl	%edi, 4(%eax)
	movl	%esp, 0(%eax)

	/* load the new kernel context */
	movl	0(%edx), %esp
	movl	4(%edx), %edi
	movl	8(%edx), %esi
	movl	12(%edx), %ebx
	movl	16(%edx), %ebp
	movl	20(%edx), %ecx
	movl	%ecx, 0(%esp)

	xor	%eax, %eax
	ret

hswitch:
	movl	4(%esp), %eax	/* %eax <- from */
	/* load the new kernel context */
	movl	0(%eax), %esp
	movl	4(%eax), %edi
	movl	8(%eax), %esi
	movl	12(%eax), %ebx
	movl	16(%eax), %ebp
	movl	20(%eax), %ecx
	movl	%ecx, 0(%esp)

	xor	%eax, %eax
	ret
