#ifdef _WIN32
#include <windows.h>

// damn windows, u just made me to do this
int main(int argc, char** argv);

// WinMain wrapper
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    return main(__argc, __argv);
}
#endif
