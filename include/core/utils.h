#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <log.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

// reads a file and stores it in a string
static inline char* read_file(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) 
    { 
        log_error("ERROR... read_file() SAYS: COULDNT OPEN FILE %s!", path); 
        return NULL; 
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static int open_file_dialog(char* out_path, size_t out_size)
{
#ifdef _WIN32
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    out_path[0] = '\0';

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "glTF Files\0*.gltf;*.glb\0All Files\0*.*\0";
    ofn.lpstrFile = out_path;
    ofn.nMaxFile = (DWORD)out_size;
    ofn.lpstrTitle = "Select a model";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    return GetOpenFileNameA(&ofn) ? 1 : 0;

#else
    // zenity or kdialog
    FILE* f = popen("zenity --file-selection --file-filter='glTF files (*.gltf *.glb) | *.gltf *.glb' 2>/dev/null", "r");
    if (!f)
        f = popen("kdialog --getopenfilename . '*.gltf *.glb' 2>/dev/null", "r");
    if (!f)
        return 0;

    if (fgets(out_path, (int)out_size, f))
    {
        // strip trailing newline
        size_t len = strlen(out_path);
        if (len > 0 && out_path[len - 1] == '\n')
            out_path[len - 1] = '\0';
        pclose(f);
        return out_path[0] != '\0';
    }
    pclose(f);
    return 0;
#endif
}

#endif
