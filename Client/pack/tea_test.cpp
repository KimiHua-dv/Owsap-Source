#include <iostream>
#include <fstream>
#include <cstdint>
#include <iomanip>

constexpr uint32_t DELTA = 0x9E3779B9;
constexpr uint32_t TEA_ROUND = 32;

struct TeaKey
{
    const char* name;
    uint32_t values[4];
};

const TeaKey KEY_SETS[] =
{
    {
        "ETERPACK_KEY",
        { 843707904, 1176167288, 992328100, 1705708525 }
    },
    {
        "SECURITY_KEY",
        { 599220564, 715591183, 1382647704, 1053692977 }
    },
    {
        "ITEM_PROTOCOL",
        { 173217, 72619434, 408587239, 27973291 }
    },
    {
        "MOB_PROTOCOL",
        { 4813894, 18955, 552631, 6822045 }
    }
};

void tea_code(
    uint32_t sz,
    uint32_t sy,
    const uint32_t* key,
    uint32_t* dest
)
{
    uint32_t y = sy;
    uint32_t z = sz;
    uint32_t sum = 0;

    for (uint32_t n = TEA_ROUND; n > 0; --n)
    {
        y += (((z << 4 ^ z >> 5) + z)
            ^ (sum + key[sum & 3]));

        sum += DELTA;

        z += (((y << 4 ^ y >> 5) + y)
            ^ (sum + key[(sum >> 11) & 3]));
    }

    dest[0] = y;
    dest[1] = z;
}

void tea_decode(
    uint32_t sz,
    uint32_t sy,
    const uint32_t* key,
    uint32_t* dest
)
{
    uint32_t y = sy;
    uint32_t z = sz;
    uint32_t sum = DELTA * TEA_ROUND;

    for (uint32_t n = TEA_ROUND; n > 0; --n)
    {
        z -= (((y << 4 ^ y >> 5) + y)
            ^ (sum + key[(sum >> 11) & 3]));

        sum -= DELTA;

        y -= (((z << 4 ^ z >> 5) + z)
            ^ (sum + key[sum & 3]));
    }

    dest[0] = y;
    dest[1] = z;
}

int main(int argc, char* argv[])
{
    const char* filename =
        argc > 1 ? argv[1] : "root.idx";

    std::ifstream file(filename, std::ios::binary);

    if (!file)
    {
        std::cout << "Erro ao abrir root.idx\n";
        return 1;
    }

    file.seekg(16, std::ios::beg);

    uint32_t source[2] = {};

    file.read(
        reinterpret_cast<char*>(source),
        sizeof(source)
    );

    if (!file)
    {
        std::cout << "Erro ao ler os dados.\n";
        return 1;
    }

    std::cout << "Bloco criptografado:\n";

    for (int i = 0; i < 2; i++)
    {
        std::cout << std::hex
                  << std::setw(8)
                  << std::setfill('0')
                  << source[i] << "\n";
    }

    std::cout << "\nTestando familias de chaves:\n\n";

	uint32_t original[2] = {
		0x5A4F434D, // MCOZ
		0x12345678
	};

	uint32_t encrypted[2] = {};
	uint32_t decrypted[2] = {};

	tea_code(
		original[1],
		original[0],
		KEY_SETS[0].values,
		encrypted
	);

	tea_decode(
		encrypted[1],
		encrypted[0],
		KEY_SETS[0].values,
		decrypted
	);

	std::cout << "\nTeste interno TEA:\n";

	std::cout << std::hex
			  << "Original:    "
			  << original[0] << " " << original[1] << "\n";

	std::cout << "Criptografado: "
			  << encrypted[0] << " " << encrypted[1] << "\n";

	std::cout << "Descriptografado: "
			  << decrypted[0] << " " << decrypted[1] << "\n";

	std::cout << (
		original[0] == decrypted[0] &&
		original[1] == decrypted[1]
		? "CICLO TEA OK\n"
		: "ERRO NO CICLO TEA\n"
	); 
    for (const TeaKey& keySet : KEY_SETS)
    {
        uint32_t destination[2] = {};

        tea_decode(
            source[1],
            source[0],
            keySet.values,
            destination
        );

        std::cout << keySet.name << "\n";

        std::cout << "DWORD 0: "
                  << std::hex
                  << std::setw(8)
                  << destination[0] << "\n";

        std::cout << "DWORD 1: "
                  << std::hex
                  << std::setw(8)
                  << destination[1] << "\n";

        std::cout << "Bytes: ";

        for (int i = 0; i < 2; i++)
        {
            for (int b = 0; b < 4; b++)
            {
                uint8_t byte =
                    (destination[i] >> (b * 8)) & 0xFF;

                std::cout << std::setw(2)
                          << static_cast<unsigned>(byte)
                          << " ";
            }
        }

        if (destination[0] == 0x5A4F434D)
        {
            std::cout << "\n*** MCOZ RECUPERADO ***";
        }

        std::cout << "\n\n";
    }

    return 0;
}