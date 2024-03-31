#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

uintptr_t view_matrix = 0x008E870C;
uintptr_t zombie = 0x18E7448;
HBRUSH brush, health_brush;
HDC hdc;

struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };

DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD processId = 0;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (Process32First(snapshot, &processEntry)) {
            do {
                if (!_wcsicmp(processEntry.szExeFile, processName)) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }

        CloseHandle(snapshot);
    }

    return processId;
}
bool WorldToScreen(Vec3 pos, Vec2& screen, float matrix[16], int windowWidth, int windowHeight)
{
    Vec4 clipCoords;
    clipCoords.x = pos.x * matrix[0] + pos.y * matrix[1] + pos.z * matrix[2] + matrix[3];
    clipCoords.y = pos.x * matrix[4] + pos.y * matrix[5] + pos.z * matrix[6] + matrix[7];
    clipCoords.z = pos.x * matrix[8] + pos.y * matrix[9] + pos.z * matrix[10] + matrix[11];
    clipCoords.w = pos.x * matrix[12] + pos.y * matrix[13] + pos.z * matrix[14] + matrix[15];
    if (clipCoords.w < 0.1f)
        return false;
    Vec3 NDC;
    NDC.x = clipCoords.x / clipCoords.w; NDC.y = clipCoords.y / clipCoords.w; NDC.z = clipCoords.z / clipCoords.w;
    screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
    screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
    return true;
}
void DrawFilledRect(int x, int y, int w, int h, HDC hdc, HBRUSH brush)
{
    RECT rect = { x, y, x + w, y + h };
    FillRect(hdc, &rect, brush);
}
void DrawBorderBox(int x, int y, int w, int h, int thickness, HDC hdc, HBRUSH brush)
{

    DrawFilledRect(x, y, w, thickness, hdc, brush);

    DrawFilledRect(x, y, thickness, h, hdc, brush);

    DrawFilledRect((x + w), y, thickness, h, hdc, brush);

    DrawFilledRect(x, y + h, w + thickness, thickness, hdc, brush);
}

void DrawBorderBoxWithHealth(int x, int y, int w, int h, int thickness, int maxHealth, int currentHealth, HDC hdc, HBRUSH borderBrush, HBRUSH healthBrush)
{
    DrawFilledRect(x, y, w, thickness, hdc, borderBrush);
    DrawFilledRect(x, y, thickness, h, hdc, borderBrush);
    DrawFilledRect(x + w, y, thickness, h, hdc, borderBrush);
    DrawFilledRect(x, y + h, w + thickness, thickness, hdc, borderBrush);
    int healthBarHeight = (h * currentHealth) / maxHealth;
    DrawFilledRect((x - thickness) + w, y, 5, healthBarHeight, hdc, healthBrush);
}


int main()
{
    DWORD pid = GetProcessIdByName(L"CoDWaW.exe");
        if (pid){
            std::cout << pid << std::endl;
            std::cout << "Obtained PID" << std::endl;
        }
        else {
            std::cout << "failed" << std::endl;
            return 0;
        }

        HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (handle != NULL) {
            std::cout << "All Access Granted" << std::endl;
        }
        else {
            std::cout << "failed" << std::endl;
            return 0;
        }

    HWND hwnd = FindWindow(0, (L"Call of Duty®"));
    
    while (true) {
        DeleteObject(hdc);
        Vec2 screen_feet, screen_head;
        RECT window_rect;
        GetClientRect(hwnd, &window_rect);
        int width = window_rect.right - window_rect.left;
        int height = window_rect.bottom - window_rect.top;
        hdc = GetDC(hwnd);
        float matrix[16];
        ReadProcessMemory(handle, (LPCVOID)view_matrix, &matrix, sizeof(matrix), NULL);
        for (int i = 0; i < 32; i++) {
            uintptr_t zombie_base = zombie + (i * 0x88);
            uintptr_t zombie_ptr;
            ReadProcessMemory(handle, (LPCVOID)zombie_base, &zombie_ptr, sizeof(zombie_ptr), NULL);
            if (zombie_ptr != NULL) {
                int health;
                ReadProcessMemory(handle, (LPCVOID)(zombie_ptr + 0x1C8), &health, sizeof(health), NULL);
                if (health != 0 && health > 0) {
                    int max_health;
                    ReadProcessMemory(handle, (LPCVOID)(zombie_ptr + 0x1CC), &max_health, sizeof(max_health), NULL);
                    Vec3 zombie_feet;
                    ReadProcessMemory(handle, (LPCVOID)(zombie_ptr + 0x160), &zombie_feet, sizeof(zombie_feet), NULL);
                    if (WorldToScreen(zombie_feet, screen_feet, matrix, width, height)) {
                        Vec3 zombie_head;
                        ReadProcessMemory(handle, (LPCVOID)(zombie_ptr + 0x154), &zombie_head, sizeof(zombie_head), NULL);
                        if (WorldToScreen(zombie_head, screen_head, matrix, width, height)) {
                            float head = screen_head.y - screen_feet.y;
                            float width_ = head / 2;
                            float center = width_ / -2;
                            brush = CreateSolidBrush(RGB(255, 0, 0));
                            health_brush = CreateSolidBrush(RGB(0, 255, 0));
                            DrawBorderBox(screen_feet.x + center, screen_feet.y, width_, head - 5, 1, hdc, brush);
                            //use DrawBorderBoxWithHelth if you want health bar on left side of box as well.
                            DeleteObject(brush);
                            DeleteObject(health_brush);
                        }
                    }
                }
            }
        }
    }
}
