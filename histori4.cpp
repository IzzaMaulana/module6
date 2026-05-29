#include <iostream>
#include <cstdlib>

#define CLEAR_TERMINAL() std::cout << "\033[2J\033[1;1H"

#define PRESS_ANY_KEY()                              \
    do                                               \
    {                                                \
        std::cout << "\n[OK] Press ENTER to continue..."; \
        std::cin.get();                              \
    } while (0)
    
using std::cout;
using std::cin;
using std::endl;

// info dasar tiap slot memori
struct MemoryEntry {
    int typeIndex;      // 0: char*, 1: uint, 2: double
    size_t entrySize;
    size_t entryOffset;
    bool isDeleted;
};

// Data pool memori dan core neural tiap sister
struct SisterCore {
    unsigned char* memoryPool;
    size_t poolSize;
    size_t alignmentSize;
    int specialGapValue;
    size_t bumpOffset;
    MemoryEntry memoryEntries[128];
    int entryCount;
};

// Hitung panjang string 
size_t getStringLength(const char* stringPointer) {
    size_t lengthCounter = 0;
    while (stringPointer[lengthCounter] != '\0') {
        lengthCounter += 1;
    }
    return lengthCounter;
}

// Alokasi memori pakai bump allocator + alignment + special gap
bool allocateMemory(SisterCore &sister, int typeIndex, const void* dataPointer, size_t dataSize) {
    if (sister.entryCount >= 128) {
        return false;
    }

    size_t currentBump = sister.bumpOffset;
    size_t alignedOffset = currentBump;
    if (currentBump > 0) {
        alignedOffset = ((currentBump + sister.alignmentSize - 1) / sister.alignmentSize) * sister.alignmentSize;
    }

    size_t finalOffset = alignedOffset;
    if (typeIndex == 0) {
        finalOffset += sister.specialGapValue;
    }

    if (finalOffset + dataSize > sister.poolSize) {
        return false;
    }

    for (size_t byteIndex = 0; byteIndex < dataSize; ++byteIndex) {
        sister.memoryPool[finalOffset + byteIndex] = ((const unsigned char*)dataPointer)[byteIndex];
    }

    MemoryEntry &newEntry = sister.memoryEntries[sister.entryCount];
    newEntry.typeIndex = typeIndex;
    newEntry.entrySize = dataSize;
    newEntry.entryOffset = finalOffset;
    newEntry.isDeleted = false;

    sister.entryCount += 1;
    sister.bumpOffset = finalOffset + dataSize;
    return true;
}

// Hapus entri memori
void deleteMemoryEntry(SisterCore &sister, int deleteIndex) {
    if (deleteIndex < 0 || deleteIndex >= sister.entryCount || sister.memoryEntries[deleteIndex].isDeleted) {
        cout << "Index out of range!" << endl;
        return;
    }

    sister.memoryEntries[deleteIndex].isDeleted = true;

    if (deleteIndex == sister.entryCount - 1) {
        sister.bumpOffset = sister.memoryEntries[deleteIndex].entryOffset;
        sister.entryCount -= 1;
        cout << "Tail reclaimed, new Bump: " << sister.bumpOffset << endl;
    } else {
        cout << "Fragmentation prevents reclaim. Delete higher indices first!" << endl;
    }
}

void clearInputBuffer() {
    cin.clear();
    cin.ignore(1000, '\n');
}

void readInt(int &num, int maxValue) {
    while (true) {
        cout << "Choose: ";
        cin >> num;
        if (cin.fail() || cin.peek() != '\n' || num < 0 || num > maxValue) {
            cout << "  [!] INVALID input, Enter a number between 0 and" << maxValue << "!" << endl;
            cin.clear();
            while (cin.get() != '\n');
            continue;
        }
        break;
    }
}

// Validasi format NIM dari argumen awal pas jalanin program
void argumentValidation(char *args, bool &isIndexValid, bool &isPrefixValid) {
    const int validIdIndex = 11;
    const char validPrefix[]  = "F1D02";
    int argsIndex = 0;

    for(int i = 0; i < 5; i++) {
        if(args[i] != validPrefix[i]) {
            isPrefixValid = false;
            break;
        }
    }

    while(args[argsIndex] != '\0') {
        argsIndex++;
    }
    
    if(argsIndex != validIdIndex) {
        isIndexValid = false;
    }
} 

// menu utama terminal
void mainMenu() {
    cout << "============================================================\n";
    cout << "SCHRYZA RESISTANCE, RECOVERY PROTOCOL [TERMINAL: PHOENIX]\n";
    cout << "============================================================\n";
    cout << "You are CyroN's Memory Architect.\n";
    cout << "Heed the gods and heal the sisters.\n";
    cout << "------------------------------------------------------------\n";
    cout << "Menu\n";
    cout << "------------------------------------------------------------\n";
    cout << "1 - Show Historia's memories\n";
    cout << "2 - Show Mira's memories\n";
    cout << "3 - Show Victoria's memories\n";
    cout << "4 - Add memory to a sister\n";
    cout << "5 - Delete memory by index from a sister\n";
    cout << "6 - Print sisters' pool diagnostics\n";
    cout << "0 - Exit\n";
    cout << "------------------------------------------------------------\n";
}

// Cetak semua memori aktif + hitung lompatan padding alignment
void displayMemoriesGeneric(const SisterCore &sister, const char* sisterName) {
    cout << "------------------------------------------------------------\n";
    cout << "Memories of " << sisterName << "\n";
    cout << "------------------------------------------------------------\n";
    
    int activeEntriesCount = 0;
    for (int entryIndex = 0; entryIndex < sister.entryCount; ++entryIndex) {
        if (sister.memoryEntries[entryIndex].isDeleted) {
            continue;
        }
        activeEntriesCount += 1;
        cout << "[" << entryIndex << "] Type: ";
        if (sister.memoryEntries[entryIndex].typeIndex == 0) cout << "char";
        else if (sister.memoryEntries[entryIndex].typeIndex == 1) cout << "uint";
        else cout << "double";

        cout << " | Size: " << sister.memoryEntries[entryIndex].entrySize;
        cout << " | Offset: " << sister.memoryEntries[entryIndex].entryOffset;
        cout << " | Address: " << (void*)(sister.memoryPool + sister.memoryEntries[entryIndex].entryOffset);
        cout << " | Value: ";

        if (sister.memoryEntries[entryIndex].typeIndex == 0) {
            cout << "\"" << (const char*)(sister.memoryPool + sister.memoryEntries[entryIndex].entryOffset) << "\"";
            cout << " | Special Gap: +" << sister.specialGapValue;
        } else if (sister.memoryEntries[entryIndex].typeIndex == 1) {
            cout << *(const unsigned int*)(sister.memoryPool + sister.memoryEntries[entryIndex].entryOffset);
        } else {
            cout << *(const double*)(sister.memoryPool + sister.memoryEntries[entryIndex].entryOffset);
        }
        cout << "\n";

        size_t rawEndOffset = sister.memoryEntries[entryIndex].entryOffset + sister.memoryEntries[entryIndex].entrySize;
        size_t nextAlignedOffset = ((rawEndOffset + sister.alignmentSize - 1) / sister.alignmentSize) * sister.alignmentSize;
        size_t structuralJumpBytes = nextAlignedOffset - rawEndOffset;
        if (structuralJumpBytes > 0) {
            cout << "Jump: " << structuralJumpBytes << "\n";
        }
    }
    if (activeEntriesCount == 0) {
        cout << "(None)\n";
    }
    cout << "Bump: " << sister.bumpOffset << " | Pool Size: " << sister.poolSize 
         << " | Align " << sister.alignmentSize << " | Special Gap: +" << sister.specialGapValue << "\n";
}

void historiaMemories(SisterCore &historia) {
    displayMemoriesGeneric(historia, "Historia");
}

void miraMemories(SisterCore &mira) {
    displayMemoriesGeneric(mira, "Mira");
}

void victoriaMemories(SisterCore &victoria) {
    displayMemoriesGeneric(victoria, "Victoria");
}

// Menu buat nambah data memori baru ke pool
void addMemoryToSister(SisterCore &historia, SisterCore &mira, SisterCore &victoria) {
    cout << "Choose sister: 0 Historia, 1 Mira, 2 Victoria: ";
    int sisterChoice;
    cin >> sisterChoice;
    if (cin.fail() || sisterChoice < 0 || sisterChoice > 2) {
        cout << "Input out of range (0-2) Try again:\n";
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    cout << "Select type: 0 char*, 1 uint, 2 double: ";
    int memoryTypeChoice;
    cin >> memoryTypeChoice;
    if (cin.fail() || memoryTypeChoice < 0 || memoryTypeChoice > 2) {
        cout << "Invalid selection!" << endl;
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    SisterCore* targetSister = (sisterChoice == 0) ? &historia : ((sisterChoice == 1) ? &mira : &victoria);

    if (memoryTypeChoice == 0) {
        cout << "Enter string (max 511): ";
        char textBuffer[512];
        cin.getline(textBuffer, 512);
        size_t stringLength = getStringLength(textBuffer);
        if (allocateMemory(*targetSister, 0, textBuffer, stringLength + 1)) {
            if (sisterChoice == 0) {
                cout << "Historia speaks: \"Discipline. Align me to 16, and leave a " << targetSister->specialGapValue << "-byte tithe.\"" << endl;
                cout << "Added string to Historia" << endl;
            } else if (sisterChoice == 1) {
                cout << "Mira smiles: \"The resistance welcomes you warmly. 8-bytes for peace, and a " << targetSister->specialGapValue << "-byte breeze of hope.\"" << endl;
                cout << "Added string to Mira" << endl;
            } else {
                cout << "Victoria rasps: \"...I do not need your help, rebel. But the abyss... pays a " << targetSister->specialGapValue << "-byte tithe for your kindness.\"" << endl;
                cout << "Added string to Victoria" << endl;
            }
        } else {
            cout << "Not enough space! Add failed" << endl;
        }
    } else if (memoryTypeChoice == 1) {
        cout << "Enter uint value: ";
        unsigned int unsignedIntegerValue;
        cin >> unsignedIntegerValue;
        if (cin.fail()) {
            clearInputBuffer();
            return;
        }
        clearInputBuffer();
        if (allocateMemory(*targetSister, 1, &unsignedIntegerValue, 4)) {
            cout << "Daiki: \"The seeds of freedom grow silently.\"" << endl;
            if (sisterChoice == 0) cout << "Historia: \"Thank you for finding me. Align me to 16, and use a " << targetSister->specialGapValue << "-byte spark to guide the lightning.\"" << endl;
            else if (sisterChoice == 1) cout << "Mira: \"A gentle breeze shields us. 8-bytes for every life we protect.\"" << endl;
            else cout << "Victoria: \"Even in the dark, mathematics from you brings a " << targetSister->specialGapValue << "-byte glimmer of hope.\"" << endl;
        } else {
            cout << "Not enough space! Add failed" << endl;
        }
    } else {
        cout << "Enter double value: ";
        double doublePrecisionValue;
        cin >> doublePrecisionValue;
        if (cin.fail()) {
            clearInputBuffer();
            return;
        }
        clearInputBuffer();
        if (allocateMemory(*targetSister, 2, &doublePrecisionValue, 8)) {
            cout << "Cyria: \"Double the efforts: always keep a distance from the cruelty of Cyron.\"" << endl;
            if (sisterChoice == 0) cout << "Historia: \"Thank you for finding me. Align me to 16, and use a " << targetSister->specialGapValue << "-byte spark to guide the lightning.\"" << endl;
            else if (sisterChoice == 1) cout << "Mira: \"A gentle breeze shields us. 8-bytes for every life we protect.\"" << endl;
            else cout << "Victoria: \"Even in the dark, mathematics from you brings a " << targetSister->specialGapValue << "-byte glimmer of hope.\"" << endl;
        } else {
            cout << "Not enough space! Add failed" << endl;
        }
    }
}

// Menu buat hapus blok memori pakai indeks
void deleteMemoryFromSister(SisterCore &historia, SisterCore &mira, SisterCore &victoria) {
    cout << "Choose sister: 0 Historia, 1 Mira, 2 Victoria: ";
    int sisterChoice;
    cin >> sisterChoice;
    if (cin.fail() || sisterChoice < 0 || sisterChoice > 2) {
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    cout << "Enter index to delete: ";
    int numericIndexToDelete;
    cin >> numericIndexToDelete;
    if (cin.fail()) {
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    SisterCore* targetSister = (sisterChoice == 0) ? &historia : ((sisterChoice == 1) ? &mira : &victoria);
    cout << "Cyria: \"You remove a shard, but voids remain if higher shards still exist.\"" << endl;
    deleteMemoryEntry(*targetSister, numericIndexToDelete);
}

// Hitung dan cetak statistik utilitas serta kapasitas pool memori
void displayDiagnosticsGeneric(const SisterCore &sister, const char* sisterName) {
    size_t totalActiveBytes = 0;
    for (int entryIndex = 0; entryIndex < sister.entryCount; ++entryIndex) {
        if (!sister.memoryEntries[entryIndex].isDeleted) {
            totalActiveBytes += sister.memoryEntries[entryIndex].entrySize;
        }
    }
    double structuralUtilization = ((double)totalActiveBytes / sister.poolSize) * 100.0;

    cout << "------------------------------------------------------------\n";
    cout << "Diagnostics for " << sisterName << "\n";
    cout << "------------------------------------------------------------\n";
    cout << "Pool: " << (void*)sister.memoryPool << " | Size: " << sister.poolSize 
         << " | Bump: " << sister.bumpOffset << " | Align: " << sister.alignmentSize << " + Gap " << sister.specialGapValue << "\n";
    cout << "Entries: " << sister.entryCount << "\n";
    cout << "Used Slots: " << sister.entryCount << " | Used Bytes: " << totalActiveBytes << "\n";
    cout << "Utilization: " << structuralUtilization << "%\n";
}

void printSisterDiagnostics(SisterCore &historia, SisterCore &mira, SisterCore &victoria) {
    cout << "Choose sister: 0 Historia, 1 Mira, 2 Victoria: ";
    int sisterChoice;
    cin >> sisterChoice;
    if (cin.fail() || sisterChoice < 0 || sisterChoice > 2) {
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    if (sisterChoice == 0) displayDiagnosticsGeneric(historia, "Historia");
    else if (sisterChoice == 1) displayDiagnosticsGeneric(mira, "Mira");
    else displayDiagnosticsGeneric(victoria, "Victoria");
}

// Ringkasan narasi akhir dan statistik total 
void renderExitSummary(const SisterCore &historia, const SisterCore &mira, const SisterCore &victoria) {
    size_t bytesHistoria = 0, bytesMira = 0, bytesVictoria = 0;
    for (int loopIndex = 0; loopIndex < historia.entryCount; ++loopIndex) if (!historia.memoryEntries[loopIndex].isDeleted) bytesHistoria += historia.memoryEntries[loopIndex].entrySize;
    for (int loopIndex = 0; loopIndex < mira.entryCount; ++loopIndex) if (!mira.memoryEntries[loopIndex].isDeleted) bytesMira += mira.memoryEntries[loopIndex].entrySize;
    for (int loopIndex = 0; loopIndex < victoria.entryCount; ++loopIndex) if (!victoria.memoryEntries[loopIndex].isDeleted) bytesVictoria += victoria.memoryEntries[loopIndex].entrySize;

    cout << "Historia: Free at last. Alignment 16, spark guidance +" << historia.specialGapValue << ".\n";
    cout << "Pool: " << (void*)historia.memoryPool << " | Size: " << historia.poolSize << "\n";
    cout << "Entries: " << historia.entryCount << " | Used Slots: " << historia.entryCount << " | Used Bytes: " << bytesHistoria << "\n";
    cout << "Utilization: " << ((double)bytesHistoria / historia.poolSize) * 100.0 << "%\n";
    cout << "Bump: " << historia.bumpOffset << " | Align 16 + Gap " << historia.specialGapValue << "\n\n";

    cout << "Mira: Wings of the resistance. Alignment 8, breeze of hope +" << mira.specialGapValue << ".\n";
    cout << "Pool: " << (void*)mira.memoryPool << " | Size: " << mira.poolSize << "\n";
    cout << "Entries: " << mira.entryCount << " | Used Slots: " << mira.entryCount << " | Used Bytes: " << bytesMira << "\n";
    cout << "Utilization: " << ((double)bytesMira / mira.poolSize) * 100.0 << "%\n";
    cout << "Bump: " << mira.bumpOffset << " | Align 8 + Gap " << mira.specialGapValue << "\n\n";

    cout << "Victoria: Shard restored. Alignment 4, tithe of kindness +" << victoria.specialGapValue << ".\n";
    cout << "Pool: " << (void*)victoria.memoryPool << " | Size: " << victoria.poolSize << "\n";
    cout << "Entries: " << victoria.entryCount << " | Used Slots: " << victoria.entryCount << " | Used Bytes: " << bytesVictoria << "\n";
    cout << "Utilization: " << ((double)bytesVictoria / victoria.poolSize) * 100.0 << "%\n";
    cout << "Bump: " << victoria.bumpOffset << " | Align 4 + Gap " << victoria.specialGapValue << "\n\n";

    cout << "Lagta: Respect alignment.\nDaiki: Mind the flow.\nCyria: Beware of the abyss.\nXelvelt: Compare stack and heap.\n";
    cout << "Good bye. May the Koura sisters function well within your hands." << endl;
}

int main(int argc, char *argv[]) {
    if(argc == 1) {
        cout << "  Usage: ./solution.exe <student_id>\n";
        cout << "  Example: ./solution.exe F1D02510067\n";
        return 1;
    } else if (argc > 2) {
        cout << "  Error: Too many arguments\n";
        return 1;
    } 

    bool isIndexValid = true;
    bool isPrefixValid = true;

    argumentValidation(argv[1], isIndexValid, isPrefixValid);

    if (!isIndexValid && !isPrefixValid) {
        cout << "  Error: Student ID must start with F1D02\n";
        cout << "  Error: Student ID must be exactly 11 characters long.\n";
        return 1;
    } else if (!isIndexValid) {
        cout << "  Error: Student ID must be exactly 11 characters long.\n";
        return 1;
    } else if (!isPrefixValid) {
        cout << "  Error: Student ID must start with F1D02.\n";
        return 1;
    } 

    int last3Digit = ((argv[1][8] - '0') * 100) + ((argv[1][9] - '0') * 10) + (argv[1][10] - '0');

    // Alokasi memori 
    SisterCore historia;
    historia.poolSize = 1024;
    historia.alignmentSize = 16;
    historia.specialGapValue = (last3Digit % 2 != 0) ? 1 : 0;
    historia.bumpOffset = 0;
    historia.entryCount = 0;
    historia.memoryPool = (unsigned char*)malloc(historia.poolSize);

    SisterCore mira;
    mira.poolSize = 2048;
    mira.alignmentSize = 8;
    mira.specialGapValue = (last3Digit * 7 % 23) + 12;
    mira.bumpOffset = 0;
    mira.entryCount = 0;
    mira.memoryPool = (unsigned char*)malloc(mira.poolSize);

    SisterCore victoria;
    victoria.poolSize = 4096;
    victoria.alignmentSize = 4;
    victoria.specialGapValue = (last3Digit * 11 % 53) + 14;
    victoria.bumpOffset = 0;
    victoria.entryCount = 0;
    victoria.memoryPool = (unsigned char*)malloc(victoria.poolSize);

    // Isi data awal (seed data bawaan) ke dalam pool masing-masing sister
    const char* textHistoriaSeed = "Historia: Schryza will be free.";
    allocateMemory(historia, 0, textHistoriaSeed, getStringLength(textHistoriaSeed) + 1);

    const char* textMiraSeed = "Mira: The winds are changing.";
    allocateMemory(mira, 0, textMiraSeed, getStringLength(textMiraSeed) + 1);
    unsigned int uintMiraSeed = 101;
    allocateMemory(mira, 1, &uintMiraSeed, 4);

    const char* textVictoriaSeed = "Victoria: Fragment of the First Light.";
    allocateMemory(victoria, 0, textVictoriaSeed, getStringLength(textVictoriaSeed) + 1);

    while (true) {
        CLEAR_TERMINAL();
        mainMenu();
        int choice;
        readInt(choice, 6);
        clearInputBuffer();
        
        if (choice == 1) {
            historiaMemories(historia);
        } else if (choice == 2) {
            miraMemories(mira);
        } else if (choice == 3) {
            victoriaMemories(victoria);
        } else if (choice == 4) {
            addMemoryToSister(historia, mira, victoria);
        } else if (choice == 5) {
            deleteMemoryFromSister(historia, mira, victoria);
        } else if (choice == 6) {
            printSisterDiagnostics(historia, mira, victoria);
        } else {
            renderExitSummary(historia, mira, victoria);
            free(historia.memoryPool);
            free(mira.memoryPool);
            free(victoria.memoryPool);
            return 0;
        }
        PRESS_ANY_KEY();
    }
}