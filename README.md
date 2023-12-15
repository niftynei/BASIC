# BASIC - hardware modules for the Base58 World Famous LARP

This code is for the dual-purpose hash-finder and hash-verifier device used in the @base58btc
World Famous Bitcoin LARP.

It allows for a user to enter in a previous block hash, transaction commitment, time,
target, and nonce.

If a nonce is provided, the BASIC will show the blockhash produced by the inputs.

If the nonce is left empty or set to zero, the unit will begin hunting for a nonce
that will produce a blockhash underneath the given target value.

Once it finds a suitable nonce, it will display first the winning nonce. You can advance
the screen to the Blockhash that nonce hashes to with the `#` key.

Be sure to write down the winning nonce value before advancing to the Blockhash screen.

To enter a number, and advance to the next input: press the `#` key.

To erase a number: press the `*` key.

## Installing with WebInstaller

Thanks to @dni and @arcbtc we have a WebInstaller available for the LARP BASICs.

Go to [https://basic.lnbits.com/](https://basic.lnbits.com/) to install the latest
build on your own device!

Note: You'll need Chrome or a Chromium browser that supports the WebInstaller protocol.


### Credit/props

Ben Arc and the LNPoS project for the inital form factor, code, and 3D case + build instructions.

Stakamoto and the Houston Astrobits for discovering and building the initial set of BASICs for @base58btc


### Hardware needed

- <a href="https://www.aliexpress.com/item/33048962331.html">Lilygo TTGO T-Display</a>
- Keypad membrane (these <a href="https://www.aliexpress.com/item/32812109541.html">big</a> ones are easy to find, these <a href="https://www.aliexpress.com/item/1005003263865347.html">smaller</a> ones will be used for workshops by arcbtc).
- <a href="https://www.amazon.co.uk/Sourcingmap-2x40-Pins-Connector-Arduino-Prototype-Black-Silver-Tone/dp/B07DK532DP/ref=sr_1_1_sspa?dchild=1&keywords=angled+gpio+pins&qid=1633810409&sr=8-1-spons&psc=1&smid=AIF4G7PLKBOZY&spLa=ZW5jcnlwdGVkUXVhbGlmaWVyPUEzQTVQOFdIWUVaMFBBJmVuY3J5cHRlZElkPUEwMTgzMjU2Mk40RUlST0pQRlRVOCZlbmNyeXB0ZWRBZElkPUEwMzQ3MTM3WlBPNEgxSjFKMEFRJndpZGdldE5hbWU9c3BfYXRmJmFjdGlvbj1jbGlja1JlZGlyZWN0JmRvTm90TG9nQ2xpY2s9dHJ1ZQ==">Angled male/male GPIO pins</a>

### Arduino software install

- Download/install latest <a href="https://www.arduino.cc/en/software">Arduino IDE</a>
- Install ESP32 boards, using <a href="https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager">boards manager</a>
- Copy <a href="https://github.com/arcbtc/LNURLPoS/tree/main/LNURLPoS/libraries">these libraries</a> into your Arduino IDE library folder
- Plug in T-Display, from _Tools>Board>ESP32 Boards_ select **TTGO LoRa32 OLED V1**

> _Note: If using MacOS, you will need the CP210x USB to UART Bridge VCP Drivers available here https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers_
