__author__ = 'micha'

# ============================================
# IMPORTS
# ============================================
from pykeyboard import PyKeyboard
from time import sleep


# ============================================
# CLASS DEFINITION
# ============================================
class Keys:

    def __init__(self):
        self.k = PyKeyboard()
        # self.suppress_xlib_output(2)

    def press_tab(self, num=1):
        for i in range(num):
            self.k.tap_key(self.k.tab_key)

    def press_home_key(self, num=1):
        for i in range(num):
            self.k.tap_key(self.k.home_key)

    def press_end_key(self, num=1):
        for i in range(num):
            self.k.tap_key(self.k.end_key)
 
    def press_enter(self):
        self.k.tap_key(self.k.enter_key)

    def press_del(self):
        self.k.tap_key(self.k.delete_key)

    def hold_key(self, key, t=1):
        self.k.press_key(key)
        sleep(t)
        self.k.release_key(key)

    def hold_down(self, t=1):
        self.hold_key(self.k.down_key, t)

    def press_down(self, num=1):
        for i in range(num):
            self.k.tap_key(self.k.down_key)

    def press_right(self, num=1):
        for i in range(num):
            self.k.tap_key(self.k.right_key)

    def press_left(self, num=1):
        for i in range(num):
            self.k.tap_key(self.k.left_key)

    def press_up(self, num=1):
        for i in range(num):
            self.k.tap_key(self.k.up_key)

    def press_esc(self):
        self.k.tap_key(self.k.escape_key)

    def send_text(self, text):
        text = str(text)
        for letter in text:
            self.k.tap_key(letter)

    def press_ctrl_enter(self, num=1):
        for i in range(num):
            self.k.press_key(self.k.control_key)
            self.press_enter()
            self.k.release_key(self.k.control_key)

    def press_ctrl_tab(self, num=1):
        for i in range(num):
            self.k.press_key(self.k.control_key)
            self.press_tab()
            self.k.release_key(self.k.control_key)

    def press_ctrl_w(self, num=1):
        for i in range(num):
            self.k.press_key(self.k.control_key)
            self.k.tap_key('w')
            self.k.release_key(self.k.control_key)

    def press_ctrl_page_up(self, num=1):
        for i in range(num):
            self.k.press_key(self.k.control_key)
            self.k.tap_key(self.k.page_up_key)
            self.k.release_key(self.k.control_key)

    def press_ctrl_page_down(self, num=1):
        for i in range(num):
            self.k.press_key(self.k.control_key)
            self.k.tap_key(self.k.page_down_key)
            self.k.release_key(self.k.control_key)

    def press_shift_left(self, num=1):
        for i in range(num):
            self.k.press_key(self.k.shift_key)
            self.k.tap_key(self.k.left_key)
            self.k.release_key(self.k.shift_key)

    def press_shift_tab(self, num=1):
        for i in range(num):
            self.k.press_key(self.k.shift_key)
            self.k.tap_key(self.k.tab_key)
            self.k.release_key(self.k.shift_key)

    def press_alt_tab(self, num=1):
        for i in range(num):
            self.k.press_key(self.k.alt_key)
            self.press_tab()
            self.k.release_key(self.k.alt_key)

    def press_ctrl_alt_and(self, f, num=1):
        for i in range(num):
            self.k.press_key(self.k.control_key)
            self.k.press_key(self.k.alt_key)
            f()
            self.k.release_key(self.k.alt_key)
            self.k.release_key(self.k.control_key)

    def press_ctrl_alt_up(self, num=1):
        self.press_ctrl_alt_and(self.press_up, num)

    def press_ctrl_alt_down(self, num=1):
        self.press_ctrl_alt_and(self.press_down, num)

    def press_ctrl_alt_right(self, num=1):
        self.press_ctrl_alt_and(self.press_right, num)

    def press_ctrl_alt_left(self, num=1):
        self.press_ctrl_alt_and(self.press_left, num)

    def press_ctrl_and(self, letter, num=1):
        assert type(letter) is str, 'The letter has to be a string'
        for i in range(num):
            self.k.press_key(self.k.control_key)
            self.k.tap_key(letter)
            self.k.release_key(self.k.control_key)

    def press_space(self):
        self.k.tap_key(' ')

    def type(self, txt):
        self.k.type_string(txt)

    def switch_terminal_tab(self, n=1):
        self.press_ctrl_page_down(n)

    @staticmethod
    def suppress_xlib_output(num):
        for i in range(num):
            print '\r\033[1A' + 46 * ' ',
        print '\r'
