#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <time.h>

#include "arm.h"
#include "cpu.h"
#include "debugger.h"

#include "devices/armcomm.h"
#include "devices/bigmac.h"
#include "devices/e001device.h"
#include "devices/eeprom.h"
#include "devices/unknowndevice.h"

int main() {
	setvbuf(stdout, NULL, _IONBF, 0);

	Cpu *cpu = new Cpu();
	ARMComm *comm = new ARMComm(cpu);
	ARM *arm = new ARM(comm, &cpu->memory);
	auto dbg = new Debugger(cpu);

	cpu->memory.MapFile(0x800000, 0x200000, "1692_f00d.bin");

	// note: vita dram mappings are different
	cpu->memory.MapRam(0x40000000, 2 * 1024 * 1024); // 2 MB of secure ram
	cpu->memory.MapRam(0x50000000, 32 * 1024 * 1024); // 32 MB of nonsecure ram

	cpu->memory.MapDevice(0xE0000000, 0x20, comm);
	cpu->memory.MapDevice(0xE0010000, 8, new e001device());
	cpu->memory.MapDevice(0xE0020000, 8, new UnknownDevice(0xE0020000));
	cpu->memory.MapDevice(0xE0050000, 0x1000, new Bigmac(&cpu->memory));

	EEPROM *eeprom = new EEPROM();
	EEPROMProgrammer *programmer = new EEPROMProgrammer(eeprom);
	eeprom->Load("1692_eeprom.bin");
	cpu->memory.MapDevice(0xE0058000, 0x10000, eeprom);
	cpu->memory.MapDevice(0xE0030000, 0x30, programmer);

	cpu->control.pc = 0x800100;

	std::thread arm_thread(&ARM::Loop, arm);
	std::thread dbg_thread(&Debugger::Loop, dbg);

	while (1) {
		cpu->Step();
	}

	return 0;
}
