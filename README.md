# SMem
A small library that unifies Shared-Memory IPC &amp; Synchronized access across multiple platforms with a standard C++ interface similar to `std::fstream`.


# Usage:


Program #1 (Creates a memory map, locks the shared mutex and writes to the memory-map):
````C++
int main(int argc, const char * argv[]) {
    
    //Map a chunk of memory..
    MemoryMap<char> map("/users/brandonanthony/Desktop/map.memory", 1024, std::ios::in | std::ios::out);
    if (map.open_file())
    {
        std::cout<<"Opened Memory File..\n";
        if (map.map())
        {
            std::cout<<"Mapped Memory File..\n";
        }
    }
    
    
    unsigned char* ptr = static_cast<unsigned char*>(map.data());
    
    //Create a shared mutex..
    Mutex *mutex = new Mutex{ptr};
    std::cout<<"LOCKED: "<<mutex->lock()<<"\n";
    
    //Write to the memory..
    ptr += 100;
    strcpy(reinterpret_cast<char*>(ptr), "Hello Friend");
    
    mutex->unlock();
    delete mutex;
    
    return 0;
}
````


Program #2 (Maps the same memory, locks the shared mutex and reads from the memory-map):
````C++
int main(int argc, const char * argv[]) {
    
    //Map the chunk of memory opened by another program..
    MemoryMap<char> map("/users/brandonanthony/Desktop/map.memory", std::ios::in | std::ios::out);
    if (map.open_file())
    {
        std::cout<<"Opened Memory File..\n";
        if (map.map())
        {
            std::cout<<"Mapped Memory File..\n";
        }
    }
    
    
    unsigned char* ptr = static_cast<unsigned char*>(map.data());
    
    //Create a shared mutex..
    Mutex *mutex = new Mutex{ptr};
    std::cout<<"LOCKED: "<<mutex->lock()<<"\n";
    ptr += 100;
    
    //Read what was written in the memory..
    printf("READ: %s\n", ptr);

    mutex->unlock();
    delete mutex;
    
    return 0;
}
