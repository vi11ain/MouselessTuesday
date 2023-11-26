// Includes
#include <Windows.h>

// Constants
#define _WIN32_WINNT 0x0500
#define MOUSE_USE_MESSAGEBOX_TITLE (L"Mouseless Tuesday!")
#define MOUSE_USE_MESSAGEBOX_CONTENT (L"Howdy!\n\nThis is Mouseless Tuesday and you may not use your mouse!")
#define TUESDAY 2

// Globals
HHOOK g_mouse_hook = NULL;
BOOL g_is_messagebox_shown = FALSE;

// Enums
typedef enum return_code_e {
    RETURN_CODE_ERROR = -1,
    RETURN_CODE_SUCCESS = 0,

    RETURN_CODE_mouse_block_BlockInput_failed,
    RETURN_CODE_mouse_block_MessageBoxW_failed,
    RETURN_CODE_mouse_block_CallNextHookEx_failed,

    RETURN_CODE_install_hook_failed,
    RETURN_CODE_install_hook_SetWindowsHookEx_failed,

    RETURN_CODE_uninstall_hook_failed,
    RETURN_CODE_uninstall_hook_UnhookWindowsHookEx_failed,
    
    RETURN_CODE_GetConsoleWindow_failed,
} return_code_t;

// Functions
LRESULT CALLBACK block_mouse_events(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (0 <= nCode) {
        // Check if a message box is already shown
        if (!g_is_messagebox_shown) {
            // Set the flag to indicate that a message box is shown
            g_is_messagebox_shown = TRUE;

            // Mouse movement detected
            MessageBox(NULL, MOUSE_USE_MESSAGEBOX_CONTENT, MOUSE_USE_MESSAGEBOX_TITLE, MB_OK | MB_ICONEXCLAMATION);

            // Reset the flag after the message box is closed
            g_is_messagebox_shown = FALSE;
        }

        // Block the mouse movement by returning 1
        return 1;
    }

    // Call the next hook procedure in the hook chain
    return CallNextHookEx(g_mouse_hook, nCode, wParam, lParam);
}

return_code_t install_hook()
{
    return_code_t return_code = RETURN_CODE_install_hook_failed;

    // Set up the mouse hook
    g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, block_mouse_events, NULL, 0);
    if (NULL == g_mouse_hook)
    {
        return_code = RETURN_CODE_install_hook_SetWindowsHookEx_failed;
        goto l_cleanup;
    }

    return_code = RETURN_CODE_SUCCESS;

l_cleanup:
    return return_code;
}

return_code_t uninstall_hook()
{
    return_code_t return_code = RETURN_CODE_uninstall_hook_failed;

    if (!UnhookWindowsHookEx(g_mouse_hook))
    {
        return_code = RETURN_CODE_uninstall_hook_UnhookWindowsHookEx_failed;
        goto l_cleanup;
    }

    g_mouse_hook = NULL;
    return_code = RETURN_CODE_SUCCESS;

l_cleanup:
    return return_code;
}

return_code_t hook_check()
{
    return_code_t return_code = RETURN_CODE_ERROR;
    SYSTEMTIME system_time = { 0 };

    GetLocalTime(&system_time);
    if (TUESDAY == system_time.wDayOfWeek)
    {
        if (NULL == g_mouse_hook)
        {
            // Day is tuesday and hook was not installed, install it!
            return_code = install_hook();
            if (RETURN_CODE_SUCCESS != return_code)
            {
                goto l_cleanup;
            }
        }
    }
    else
    {
        if (NULL != g_mouse_hook)
        {
            // Day is not tuesday and hook is installed, uninstall it!
            return_code = uninstall_hook();
            if (RETURN_CODE_SUCCESS != return_code)
            {
                goto l_cleanup;
            }
        }
    }

    return_code = RETURN_CODE_SUCCESS;

l_cleanup:
    return return_code;
}

INT wmain(VOID)
{
    return_code_t return_code = RETURN_CODE_ERROR;
    HWND console_window = NULL;
    MSG message = { 0 };

    console_window = GetConsoleWindow();
    if (NULL == console_window)
    {
        return_code = RETURN_CODE_GetConsoleWindow_failed;
        goto l_cleanup;
    }

    // Hide console window
    ShowWindow(console_window, SW_MINIMIZE);
    ShowWindow(console_window, SW_HIDE);

    return_code = hook_check();
    if (RETURN_CODE_SUCCESS != return_code) {
        goto l_cleanup;
    }

l_wait:
    // If hook is not installed, check and perform whether it should be installed
    if (NULL == g_mouse_hook)
    {
        return_code = hook_check();
        if (RETURN_CODE_SUCCESS != return_code) {
            goto l_cleanup;
        }
    }

    // If hook was installed, go into message loop
    if (NULL != g_mouse_hook)
    {
        // Message loop to keep the application running
        while (GetMessage(&message, NULL, 0, 0))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);

            return_code = hook_check();
            if (RETURN_CODE_SUCCESS != return_code) {
                goto l_cleanup;
            }

            if (NULL == g_mouse_hook)
            {
                // If hook was uninstalled, escape message loop back to waiting loop
                goto l_wait;
            }
        }
    }

    // Sleep for 5 hours and check again
    Sleep(1000 * 60 * 60 * 5);
    goto l_wait;

    return_code = RETURN_CODE_SUCCESS;

l_cleanup:
    return (INT)return_code;
}