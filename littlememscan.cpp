
#include <iostream>
#include <Windows.h>
#include <winnt.h>
#include <vector>
int main(int argc, const char* argv[]) {
	std::cout << "POC Memory Scanner by lowlevel01 :v" << std::endl;
	//int pid = 11876;
	int pid = std::atoi(argv[1]);
	int targetValue;
	std::cout << "Enter an initial value to look for: "; std::cin >> targetValue;

	std::vector<void*> foundAddresses;

	HANDLE hProc = ::OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (hProc == NULL) {
		std::cout << "Couldn't obtain a handle on the process : " << ::GetLastError() << std::endl;
		return 0;
	}

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	LPVOID minAppAddress = sysInfo.lpMinimumApplicationAddress;
	LPVOID maxAppAddress = sysInfo.lpMaximumApplicationAddress;

	MEMORY_BASIC_INFORMATION memInfo;
	size_t bytesRead;
	unsigned char buffer[4096];

	for (LPVOID address = minAppAddress; address < maxAppAddress;) {
		if (!::VirtualQueryEx(hProc, address, &memInfo, sizeof(memInfo))) {
			std::cout << "Couldn't Query Memory : " << ::GetLastError() << std::endl;
			return 0;
		}

		if (memInfo.State == MEM_COMMIT && (memInfo.Protect & PAGE_READWRITE)) {
			for (size_t offset = 0; offset < memInfo.RegionSize; offset += sizeof(buffer)) {
				size_t read_size = min(sizeof(buffer), memInfo.RegionSize - offset);
				if (ReadProcessMemory(hProc, (BYTE*)address + offset, buffer, read_size, &bytesRead)) {
					for (size_t i = 0; i < bytesRead - sizeof(int); i++) {
						int value = *reinterpret_cast<int*>(&buffer[i]);
						if (value == targetValue) {
							void* foundAddress = (void*)((BYTE*)address + offset + i);
							foundAddresses.push_back(foundAddress);
							std::cout << "Found value: " << value << " At address :" << foundAddress << std::endl;
						}
					}
				}
			}
		}
		address = (LPVOID)((SIZE_T)address + memInfo.RegionSize);
	}

	std::cout << "Now introduce a change. Enter new value: "; std::cin >> targetValue;
	
	for (const auto& address : foundAddresses) {
		int value;
		size_t bytesRead;
		if (ReadProcessMemory(hProc, address, &value, sizeof(value), &bytesRead) && bytesRead == sizeof(value)) {
			std::cout << "Value at address " << address << ": " << value << std::endl;
			if (value == targetValue) {
				std::cout << "Caught a change!" << std::endl;
				int newValue;
				size_t bytesWritten;
				std::cout << "Enter a new value to insert: "; std::cin >> newValue;
				if (WriteProcessMemory(hProc, address, &newValue, sizeof(newValue), &bytesWritten) && bytesWritten == sizeof(newValue)) {
					std::cout << "Successfully wrote " << newValue << " to address " << address << std::endl;
				}
			}
		}
		else {
			std::cerr << "Failed to read memory at address: " << address << std::endl;
		}
	}
	return 0;

}
