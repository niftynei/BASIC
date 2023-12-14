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
