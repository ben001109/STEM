from flask import Flask, jsonify
from SX127x.LoRa import *
from SX127x.board_config import BOARD
import threading

app = Flask(__name__)

# 存儲最新的 LoRa 數據
lora_data = {}

# 設置 LoRa
BOARD.setup()
lora = LoRaRcvCont(verbose=False)
lora.set_mode(MODE.SLEEP)
lora.set_dio_mapping([0] * 6)


def on_receive(payload):
    global lora_data
    lora_data = payload
    print("Received data:", payload)


lora.on_rx_done = on_receive


def start_lora():
    lora.set_mode(MODE.RXCONT)


@app.route('/get-lora-data')
def get_lora_data():
    return jsonify(lora_data)


if __name__ == '__main__':
    threading.Thread(target=start_lora).start()
    app.run(host='0.0.0.0', port=5000)
