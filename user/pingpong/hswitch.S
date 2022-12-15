
	.globl hswitch

hswitch:
	movl	4(%esp), %eax /* holds handler address */

	/* save old user context */
	movl	%ecx, 24(%esp)
	movl	%ebp, 20(%esp)
	movl	%ebx, 16(%esp)
	movl	%esi, 12(%esp)
	movl	%edi, 8(%esp)

	call *%eax

	movl	8(%esp), %edi
	movl	12(%esp), %esi
	movl	16(%esp), %ebx
	movl	20(%esp), %ebp
	movl	24(%esp), %ecx