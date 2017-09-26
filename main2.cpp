//
//  main.cpp
//  IPC
//
//  Created by Brandon on 2017-09-24.
//  Copyright © 2017 Brandon. All rights reserved.
//

#include <iostream>
#include <thread>

#include "SharedEvent.hxx"
#include "MemoryMap.hxx"

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
    std::cout<<"LOCKED: "<<mutex->timed_lock(1000)<<"\n";
    ptr += 100;
    
    //Read what was written in the memory..
    printf("READ: %s\n", ptr);

    mutex->unlock();
    delete mutex;
    
    return 0;
}
