#include "qtmackeymapper.h"

#ifdef Q_OS_MAC

#include <Carbon/Carbon.h>

QtMacKeyMapper::QtMacKeyMapper(QObject *parent)
    : QtKeyMapperBase(parent)
{
    populateMappingHashes();
    populateCharKeyInformation();
    identifier = QStringLiteral("macos");
}

void QtMacKeyMapper::populateMappingHashes()
{
    qtKeyToVirtKeyHash.clear();
    virtKeyToQtKeyHash.clear();

    auto mapKey = [this](int qtKey, int virtualKey) {
        qtKeyToVirtKeyHash.insert(qtKey, virtualKey);
        virtKeyToQtKeyHash.insert(virtualKey, qtKey);
    };

    // Alphabet keys
    mapKey(Qt::Key_A, kVK_ANSI_A);
    mapKey(Qt::Key_S, kVK_ANSI_S);
    mapKey(Qt::Key_D, kVK_ANSI_D);
    mapKey(Qt::Key_F, kVK_ANSI_F);
    mapKey(Qt::Key_H, kVK_ANSI_H);
    mapKey(Qt::Key_G, kVK_ANSI_G);
    mapKey(Qt::Key_Z, kVK_ANSI_Z);
    mapKey(Qt::Key_X, kVK_ANSI_X);
    mapKey(Qt::Key_C, kVK_ANSI_C);
    mapKey(Qt::Key_V, kVK_ANSI_V);
    mapKey(Qt::Key_B, kVK_ANSI_B);
    mapKey(Qt::Key_Q, kVK_ANSI_Q);
    mapKey(Qt::Key_W, kVK_ANSI_W);
    mapKey(Qt::Key_E, kVK_ANSI_E);
    mapKey(Qt::Key_R, kVK_ANSI_R);
    mapKey(Qt::Key_Y, kVK_ANSI_Y);
    mapKey(Qt::Key_T, kVK_ANSI_T);
    mapKey(Qt::Key_U, kVK_ANSI_U);
    mapKey(Qt::Key_I, kVK_ANSI_I);
    mapKey(Qt::Key_O, kVK_ANSI_O);
    mapKey(Qt::Key_P, kVK_ANSI_P);
    mapKey(Qt::Key_L, kVK_ANSI_L);
    mapKey(Qt::Key_J, kVK_ANSI_J);
    mapKey(Qt::Key_K, kVK_ANSI_K);
    mapKey(Qt::Key_M, kVK_ANSI_M);
    mapKey(Qt::Key_N, kVK_ANSI_N);

    // Number row and associated symbols
    mapKey(Qt::Key_1, kVK_ANSI_1);
    mapKey(Qt::Key_2, kVK_ANSI_2);
    mapKey(Qt::Key_3, kVK_ANSI_3);
    mapKey(Qt::Key_4, kVK_ANSI_4);
    mapKey(Qt::Key_5, kVK_ANSI_5);
    mapKey(Qt::Key_6, kVK_ANSI_6);
    mapKey(Qt::Key_7, kVK_ANSI_7);
    mapKey(Qt::Key_8, kVK_ANSI_8);
    mapKey(Qt::Key_9, kVK_ANSI_9);
    mapKey(Qt::Key_0, kVK_ANSI_0);
    mapKey(Qt::Key_Minus, kVK_ANSI_Minus);
    mapKey(Qt::Key_Equal, kVK_ANSI_Equal);

    // Punctuation
    mapKey(Qt::Key_BracketLeft, kVK_ANSI_LeftBracket);
    mapKey(Qt::Key_BracketRight, kVK_ANSI_RightBracket);
    mapKey(Qt::Key_Backslash, kVK_ANSI_Backslash);
    mapKey(Qt::Key_Semicolon, kVK_ANSI_Semicolon);
    mapKey(Qt::Key_Apostrophe, kVK_ANSI_Quote);
    mapKey(Qt::Key_Comma, kVK_ANSI_Comma);
    mapKey(Qt::Key_Period, kVK_ANSI_Period);
    mapKey(Qt::Key_Slash, kVK_ANSI_Slash);
    mapKey(Qt::Key_QuoteLeft, kVK_ANSI_Grave);

    // Special keys
    mapKey(Qt::Key_Return, kVK_Return);
    mapKey(Qt::Key_Enter, kVK_Return);
    mapKey(Qt::Key_Tab, kVK_Tab);
    mapKey(Qt::Key_Space, kVK_Space);
    mapKey(Qt::Key_Backspace, kVK_Delete);
    mapKey(Qt::Key_Escape, kVK_Escape);
    mapKey(Qt::Key_CapsLock, kVK_CapsLock);
    mapKey(Qt::Key_Shift, kVK_Shift);
    mapKey(AntKey_Shift_R, kVK_RightShift);
    mapKey(Qt::Key_Control, kVK_Control);
    mapKey(Qt::Key_Meta, kVK_Command);
    mapKey(Qt::Key_Alt, kVK_Option);
    mapKey(AntKey_Alt_R, kVK_RightOption);
    mapKey(AntKey_Control_R, kVK_RightControl);

    mapKey(Qt::Key_Left, kVK_LeftArrow);
    mapKey(Qt::Key_Right, kVK_RightArrow);
    mapKey(Qt::Key_Up, kVK_UpArrow);
    mapKey(Qt::Key_Down, kVK_DownArrow);
    mapKey(Qt::Key_Home, kVK_Home);
    mapKey(Qt::Key_End, kVK_End);
    mapKey(Qt::Key_PageUp, kVK_PageUp);
    mapKey(Qt::Key_PageDown, kVK_PageDown);
    mapKey(Qt::Key_Delete, kVK_ForwardDelete);
    mapKey(Qt::Key_Help, kVK_Help);

    // Function keys
    mapKey(Qt::Key_F1, kVK_F1);
    mapKey(Qt::Key_F2, kVK_F2);
    mapKey(Qt::Key_F3, kVK_F3);
    mapKey(Qt::Key_F4, kVK_F4);
    mapKey(Qt::Key_F5, kVK_F5);
    mapKey(Qt::Key_F6, kVK_F6);
    mapKey(Qt::Key_F7, kVK_F7);
    mapKey(Qt::Key_F8, kVK_F8);
    mapKey(Qt::Key_F9, kVK_F9);
    mapKey(Qt::Key_F10, kVK_F10);
    mapKey(Qt::Key_F11, kVK_F11);
    mapKey(Qt::Key_F12, kVK_F12);
    mapKey(Qt::Key_F13, kVK_F13);
    mapKey(Qt::Key_F14, kVK_F14);
    mapKey(Qt::Key_F15, kVK_F15);
    mapKey(Qt::Key_F16, kVK_F16);
    mapKey(Qt::Key_F17, kVK_F17);
    mapKey(Qt::Key_F18, kVK_F18);
    mapKey(Qt::Key_F19, kVK_F19);
    mapKey(Qt::Key_F20, kVK_F20);

    // Volume / media keys
    mapKey(Qt::Key_VolumeUp, kVK_VolumeUp);
    mapKey(Qt::Key_VolumeDown, kVK_VolumeDown);
    mapKey(Qt::Key_VolumeMute, kVK_Mute);

    // Keypad
    mapKey(AntKey_KP_0, kVK_ANSI_Keypad0);
    mapKey(AntKey_KP_1, kVK_ANSI_Keypad1);
    mapKey(AntKey_KP_2, kVK_ANSI_Keypad2);
    mapKey(AntKey_KP_3, kVK_ANSI_Keypad3);
    mapKey(AntKey_KP_4, kVK_ANSI_Keypad4);
    mapKey(AntKey_KP_5, kVK_ANSI_Keypad5);
    mapKey(AntKey_KP_6, kVK_ANSI_Keypad6);
    mapKey(AntKey_KP_7, kVK_ANSI_Keypad7);
    mapKey(AntKey_KP_8, kVK_ANSI_Keypad8);
    mapKey(AntKey_KP_9, kVK_ANSI_Keypad9);
    mapKey(AntKey_KP_Decimal, kVK_ANSI_KeypadDecimal);
    mapKey(AntKey_KP_Add, kVK_ANSI_KeypadPlus);
    mapKey(AntKey_KP_Subtract, kVK_ANSI_KeypadMinus);
    mapKey(AntKey_KP_Multiply, kVK_ANSI_KeypadMultiply);
    mapKey(AntKey_KP_Divide, kVK_ANSI_KeypadDivide);
    mapKey(AntKey_KP_Enter, kVK_ANSI_KeypadEnter);
    mapKey(AntKey_KP_Begin, kVK_ANSI_KeypadClear);
}

void QtMacKeyMapper::populateCharKeyInformation()
{
    virtkeyToCharKeyInfo.clear();

    auto addCharacter = [this](QChar value, int qtKey, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        const int virtualKey = qtKeyToVirtKeyHash.value(qtKey, 0);
        if (virtualKey <= 0)
            return;

        charKeyInformation info;
        info.modifiers = modifiers;
        info.virtualkey = virtualKey;
        virtkeyToCharKeyInfo.insert(value.unicode(), info);
    };

    for (ushort code = 'a'; code <= 'z'; ++code)
    {
        QChar ch(code);
        const int qtKey = Qt::Key_A + (ch.unicode() - 'a');
        addCharacter(ch, qtKey, Qt::NoModifier);
        addCharacter(ch.toUpper(), qtKey, Qt::ShiftModifier);
    }

    for (ushort code = '0'; code <= '9'; ++code)
    {
        QChar digit(code);
        const int qtKey = Qt::Key_0 + (digit.unicode() - '0');
        addCharacter(digit, qtKey, Qt::NoModifier);
    }

    addCharacter(' ', Qt::Key_Space);
    addCharacter('-', Qt::Key_Minus);
    addCharacter('_', Qt::Key_Minus, Qt::ShiftModifier);
    addCharacter('=', Qt::Key_Equal);
    addCharacter('+', Qt::Key_Equal, Qt::ShiftModifier);
    addCharacter('[', Qt::Key_BracketLeft);
    addCharacter('{', Qt::Key_BracketLeft, Qt::ShiftModifier);
    addCharacter(']', Qt::Key_BracketRight);
    addCharacter('}', Qt::Key_BracketRight, Qt::ShiftModifier);
    addCharacter(';', Qt::Key_Semicolon);
    addCharacter(':', Qt::Key_Semicolon, Qt::ShiftModifier);
    addCharacter('\'', Qt::Key_Apostrophe);
    addCharacter('"', Qt::Key_Apostrophe, Qt::ShiftModifier);
    addCharacter(',', Qt::Key_Comma);
    addCharacter('<', Qt::Key_Comma, Qt::ShiftModifier);
    addCharacter('.', Qt::Key_Period);
    addCharacter('>', Qt::Key_Period, Qt::ShiftModifier);
    addCharacter('/', Qt::Key_Slash);
    addCharacter('?', Qt::Key_Slash, Qt::ShiftModifier);
    addCharacter('`', Qt::Key_QuoteLeft);
    addCharacter('~', Qt::Key_QuoteLeft, Qt::ShiftModifier);
}

#endif // Q_OS_MAC
