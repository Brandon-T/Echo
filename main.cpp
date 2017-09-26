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
