import sys
import asyncio
import websockets
from PyQt5 import uic, QtGui
from PyQt5.QtWidgets import QApplication, QWidget
from qasync import QEventLoop, asyncSlot

ESP32_WS_URL = "ws://10.225.226.26/ws"


class LEDControl(QWidget):
    def __init__(self):
        super().__init__()
        uic.loadUi("ui\led_control.ui", self)
        for name in ("status_label", "button_on", "button_off", "led_indicator", "log_view", "button_ws"):
            setattr(self, name, self.findChild(type(self.findChild(QWidget, name)), name))

        self.set_led_image("off")

        self.button_on.clicked.connect(lambda: self.send_command("on"))
        self.button_off.clicked.connect(lambda: self.send_command("off"))
        self.button_ws.clicked.connect(lambda: asyncio.ensure_future(self.connect_ws()))

        self.ws = None
        self.is_running = True
        asyncio.ensure_future(self.connect_ws())
        asyncio.ensure_future(self.ws_status_printer())

    def set_led_image(self, state):
        pixmap = QtGui.QPixmap(f"ui\led_{state}.png")
        self.led_indicator.setPixmap(pixmap)

    def append_log(self, message):
        self.log_view.appendPlainText(message.strip())
        self.log_view.verticalScrollBar().setValue(self.log_view.verticalScrollBar().maximum())

    async def connect_ws(self):
        if self.ws and self.ws.open:
            self.status_label.setText("Already connected")
            return

        while self.is_running:
            try:
                self.ws = await websockets.connect(ESP32_WS_URL)
                self.status_label.setText("Connected to ESP32")
                async for message in self.ws:
                    self.append_log(message)
            except Exception as e:
                self.status_label.setText(f"Error: {e}")
                self.ws = None
                await asyncio.sleep(1)

    async def ws_status_printer(self):
        """Раз у 6 секунд писати стан з'єднання"""
        while self.is_running:
            if self.ws and self.ws.open:
                self.append_log("[WS STATUS] Connected")
            else:
                self.append_log("[WS STATUS] Disconnected")
            await asyncio.sleep(6)

    @asyncSlot()
    async def send_command(self, command):
        if self.ws and self.ws.open:
            try:
                await self.ws.send(command)
                self.status_label.setText(f"LED state: {command}")
                self.set_led_image("on" if command == "on" else "off")
            except Exception as e:
                self.status_label.setText(f"Error while sending: {e}")
        else:
            self.status_label.setText("WebSocket isn't connected")

    def closeEvent(self, event):
        self.is_running = False
        if self.ws:
            asyncio.ensure_future(self.ws.close())
        event.accept()


def main():
    app = QApplication(sys.argv)
    loop = QEventLoop(app)
    asyncio.set_event_loop(loop)

    window = LEDControl()
    window.show()

    with loop:
        loop.run_forever()


if __name__ == "__main__":
    main()
