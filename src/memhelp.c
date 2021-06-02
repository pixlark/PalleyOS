#include <stddef.h>
#include <stdint.h>

// Theoretical Mapping from Virtual to Physical Address
intptr_t vtp(uint32_t virtualaddr) {
	term_write("Virtual Addr: ");
	term_write_uint32(virtualaddr, 16);
	uint32_t offset = virtualaddr & (0x3fffff); // Last 22 bits
	uint32_t pde_index = virtualaddr & (0xffc << 20);
	pde_index >>= 20;

	term_write(", index: ");
	term_write_uint32(pde_index, 16);

	term_write(", pde: ");
	term_write_uint32(page_directory[pde_index], 16);
	term_write(" ");

	// 39:32 of final, 20:13 of pde
	uint32_t upper = page_directory[pde_index] & (0xf << 13);

	term_write("upper: ");
	term_write_uint32(upper, 16);
	term_write(" ");

	// 31:22 of final, 31:22 of pde
	uint32_t lower = page_directory[pde_index] & (0xffc << 20);

	term_write("\nlower: ");
	term_write_uint32(lower, 16);
	term_write(" ");

	intptr_t physaddrcpy = upper << 15 | lower | offset;
	term_write("PhysAddr: ");
	term_write_uint32(physaddrcpy, 16);
	term_write("\n");

	return physaddrcpy;
}
