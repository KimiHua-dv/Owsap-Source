
import struct
import sys
from pathlib import Path

# ============================================================
# CONFIGURAÇÃO — CHAVES DO OWSAP (ServiceDefs.h)
# ============================================================

KEY_SETS = {
	"ETERPACK_KEY": [
		843707904,
		1176167288,
		992328100,
		1705708525
	],

	"ETERPACK_SECURITY_KEY": [
		599220564,
		715591183,
		1382647704,
		1053692977
	]
}

DELTA = 0x9E3779B9
ROUNDS = 32
MASK32 = 0xFFFFFFFF

MAGIC = b"MCOZ"

ETERPACK_KEY = [
    843707904,
    1176167288,
    992328100,
    1705708525
]

# ============================================================
# TEA DECRYPT — REPRODUÇÃO DO tea_decode() DO OWSAP
# ============================================================


def tea_decode(sz, sy, key):

	y = sy
	z = sz

	total = (DELTA * ROUNDS) & MASK32

	for _ in range(ROUNDS):

		# Reprodução exata das operações unsigned long de 32 bits
		parte1 = (((y << 4) & MASK32) ^ (y >> 5))
		parte1 = (parte1 + y) & MASK32

		parte2 = (total + key[(total >> 11) & 3]) & MASK32

		z = (z - (parte1 ^ parte2)) & MASK32

		total = (total - DELTA) & MASK32

		parte1 = (((z << 4) & MASK32) ^ (z >> 5))
		parte1 = (parte1 + z) & MASK32

		parte2 = (total + key[total & 3]) & MASK32

		y = (y - (parte1 ^ parte2)) & MASK32

	return y, z

# ============================================================
# DESCRIPTOGRAFAR BUFFER COMPLETO
# ============================================================

def tea_decrypt(data, key):

	if len(data) % 8 != 0:
		raise ValueError("O tamanho precisa ser múltiplo de 8.")

	output = bytearray()

	for offset in range(0, len(data), 8):

		# Equivalente à leitura de dois DWORDs no C++
		word0, word1 = struct.unpack_from("<II", data, offset)

		# Equivalente a:
		# tea_decode(*(src + 1), *src, key, dest);
		result0, result1 = tea_decode(word1, word0, key)

		output.extend(struct.pack("<II", result0, result1))

	return bytes(output)


# ============================================================
# LEITURA E TESTE DO ROOT.IDX
# ============================================================


def main():

    caminho = Path(
        sys.argv[1] if len(sys.argv) > 1 else "root.idx"
    )

    arquivo = caminho.read_bytes()

    magic, encrypt_size, compressed_size, real_size = (
        struct.unpack_from("<4I", arquivo, 0)
    )

    print("Cabeçalho:", magic.to_bytes(4, "little"))
    print("EncryptSize:", encrypt_size)
    print("CompressedSize:", compressed_size)
    print("RealSize:", real_size)

    for offset in (16, 20):

        dados_criptografados = arquivo[
            offset:offset + encrypt_size
        ]

        if len(dados_criptografados) != encrypt_size:
            continue

        for inverter in (False, True):

            resultado = bytearray()

            for pos in range(0, len(dados_criptografados), 8):

                a, b = struct.unpack_from(
                    "<II", dados_criptografados, pos
                )

                if inverter:
                    x, y = tea_decode(a, b, ETERPACK_KEY)
                else:
                    x, y = tea_decode(b, a, ETERPACK_KEY)

                resultado.extend(struct.pack("<II", x, y))

            assinatura = bytes(resultado[:4])

            print(
                f"Offset={offset} | "
                f"Inverter={inverter} | "
                f"Resultado={assinatura.hex(' ')}"
            )

            if assinatura == MAGIC:
                print("\nMCOZ RECUPERADO!")
                caminho_saida = caminho.with_name(
                    "root.idx.decrypted.bin"
                )
                caminho_saida.write_bytes(resultado)
                print("Arquivo salvo:", caminho_saida)


if __name__ == "__main__":
    main()
