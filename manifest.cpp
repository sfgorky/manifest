#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <set>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <functional>


static
bool dirExists(const std::string& path)
{
    struct stat info;

    if(stat( path.c_str(), &info ) != 0)
        return false;
    else if(info.st_mode & S_IFDIR)
        return true;
    else
        return false;
}

static 
bool fileExist(const std::string& fname)
{
    bool bOk = false;
    FILE* f = fopen(fname.c_str(), "r");
   
    if(f)
    {
        bOk = true;
        fclose(f);
    }

    return bOk;
}

///////////////////////////////////////////////////////////////////////////////

static
std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::ptr_fun<int, int>(std::isgraph)));
    return s;
}

///////////////////////////////////////////////////////////////////////////////

static 
std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
    std::ptr_fun<int, int>(std::isgraph)).base(), s.end());
    return s;
}

///////////////////////////////////////////////////////////////////////////////

static 
std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}

///////////////////////////////////////////////////////////////////////////////

typedef std::set<std::string> ManifestList;

bool ReadManifest(const std::string& fname, ManifestList& manifestList)
{
    bool bOk(true);
    
    std::string line;
    std::ifstream f(fname);
    if(f.is_open())
    {
        while( getline(f, line))
        {
            trim(line);
            if(line.size()==0 || line[0] == '#') continue;
            
            //fprintf(stdout, "%s\n", line.c_str());
            manifestList.insert(line);
            
        }
        f.close();
    }
    else
    {
        bOk = false;
    }
    return bOk;
}

///////////////////////////////////////////////////////////////////////////////

static
std::string findToken(const std::string& s, const size_t pos, const char left, const char right)
{
    std::string ret;
    
    size_t pleft  = s.find(left, pos);
    size_t pright = pleft!=std::string::npos && pleft>pos ? s.find(right, pleft+1) : std::string::npos;
    
    if(pleft != std::string::npos && pright != std::string::npos)
    {
        ret = s.substr(pleft+1, (pright-pleft-1));
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

static 
bool processFile(const std::string& fname, 
                 const ManifestList& list, 
                 bool processSystemInclude,
                 bool printIncludeDir,
                 bool verbose)
{
    const std::string includeToken("#include");
    bool bOk(true);
    std::string line;
    std::ifstream f(fname);
    if(f.is_open())
    {
        if(verbose)
        {
            fprintf(stdout, "* Processing: '%s'\n", fname.c_str());
        }
        while(getline(f, line))
        {
            std::size_t found = line.find(includeToken);

            if(found != std::string::npos)
            {
                std::string included = findToken(line, includeToken.size(), '"', '"');

                if(included.size() == 0 && processSystemInclude)
                {
                    included = findToken(line, includeToken.size(), '<', '>');    
                }
                if(!included.empty())
                {
                    bool inManifest = (list.find(included) != list.end());
                    
                    if(printIncludeDir)
                    {
                        fprintf(stdout, "include %s\n", included.c_str());
                    }                    
                    
                    if(!inManifest)
                    {
                        fprintf(stdout, "File '%s': included '%s' not presented in manifest.\n", fname.c_str(), included.c_str());
                        bOk = false;
                    }   
                }
            }
        }
        f.close();
    }
    return bOk;
}

////////////////////////////////////////////////////////////////////////////////

bool ProcessManifestFile(const std::string basedir, 
                         const std::string manifestFile,
                         const bool        printIncludeDir,
                         const bool        verbose)
{
    ManifestList manifestList;

    if(verbose)
    {
        fprintf(stdout, "Processing Manifest: '%s', basedir: '%s'\n", 
                        manifestFile.c_str(),
                        basedir.c_str());
    }
    bool bOk = ReadManifest(manifestFile, manifestList);
    
    if(bOk)
    {
        for(ManifestList::iterator it=manifestList.begin(); it!=manifestList.end(); ++it)
        {
            bool processSystemInclude = false;
            std::string fname = basedir + "/" + *it;
            bOk &= processFile(fname, manifestList, processSystemInclude, printIncludeDir, verbose);
        }
    }
    else
    {
        fprintf(stdout, "Error: cannot read manifest file '%s'\n", manifestFile.c_str());
    }
    return bOk;
}

////////////////////////////////////////////////////////////////////////////////

#define VERSION "1.0"

///////////////////////////////////////////////////////////////////////////////

static void usage(FILE* out)
{
    fprintf(out, "manifest (%s) Check for export manifest file\n", VERSION);
    fputs("manifest [-b<basdir>] -m<manifest>\n"
          "   -b base library directory\n"
          "   -m output file\n"
          "   -v verbose mode\n"
          "   -p all include directories\n"
          "\n"
          , out);
}

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    int ret(0);
    std::string basedir;
    std::string manifestFile;
    bool verbose(false);
    bool printIncludeDir(false);

    int opt = 0;
    while((opt = getopt(argc, argv, "b:m:hvp")) != -1)
    {
        switch(opt)
        {
            case 'b':
                basedir = optarg;
                break;
            case 'm':
                manifestFile = optarg;
                break;
            case 'h':
                usage(stdout);
                break;
            case 'v':
                verbose = true;
                break;
            case 'p':
                printIncludeDir = true;
                break;
            default:
                break;
        }
    }
    if(manifestFile.empty())
    {
        usage(stdout);
    }
    else
    {
        if(manifestFile.empty() || !fileExist(manifestFile))
        {
            fprintf(stdout, "Error: manifest file '%s' cannot be found.\n", manifestFile.c_str());
            ret = 1;
        }
        if(!dirExists(basedir))
        {
            fprintf(stdout, "Error: base directory does not exist. ('%s')\n", basedir.c_str());
            ret = 1;
        }
        else
        {
            bool bOk = ProcessManifestFile(basedir, manifestFile, printIncludeDir, verbose);
            ret = (bOk ? 0 : 1);

            fprintf(stdout, "\n");
            fprintf(stdout, "Status: %s\n", (bOk ? "OK" : "ERROR"));
        }
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////



