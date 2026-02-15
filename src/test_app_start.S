/* Minimal RISC-V startup for test application
 *
 * Initializes stack, then calls app_main()
 */

.section .text.init, "ax"
.globl _start
.type _start, @function

_start:
    /* Initialize stack pointer to end of RAM
     * For QEMU virt: RAM is 0x80100000 + 128K = 0x80120000 */
    li sp, 0x80120000
    
    /* Call application main */
    call app_main
    
    /* If app returns, loop forever */
    j .

