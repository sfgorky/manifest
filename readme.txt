manifest
========

The puporse of this application is to check if all the exported header files 
in a library are not included any internal header.

All exported header files must be explicitly listed in a an manifest file. 

The 'manifest' binary will scan all the header files described in the manifest
list and check that all the include statements on this files will ONLY reference
files in a list.


Example:
    Conside the following library:
    
    
    mylib
        src 
            file1.cpp
            file1.h
            file2.cpp
            file2.h
            file3.cpp
            file3.h
            
        manifest.txt
            file1.h
            file2.h
            
            
The call to:

    manifest -m manifest.txt -b <locationOf library>

will return 1:0 according to:

        1:  error:  In case any of the files in manifest.txt includes something
                    else then 'file1.h', and 'file2.h'

        0:  success
        

