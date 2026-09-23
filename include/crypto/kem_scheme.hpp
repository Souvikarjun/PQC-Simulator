#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace v2x::crypto {

struct KemKeyPair {
    std::vector<std::uint8_t> publicKey;
    std::vector<std::uint8_t> privateKey;
};

struct KemExchange {
    std::vector<std::uint8_t> ciphertext;
    std::vector<std::uint8_t> sharedSecret;
};

struct KemSizes {
    std::size_t publicKeyBytes{};
    std::size_t privateKeyBytes{};
    std::size_t ciphertextBytes{};
    std::size_t sharedSecretBytes{};
};

class KemScheme {
public:
    virtual ~KemScheme() = default;
    virtual std::string name() const = 0;
    virtual KemKeyPair generateKeyPair(std::uint64_t seed) const = 0;
    virtual KemExchange encapsulate(const std::vector<std::uint8_t>& publicKey,
                                    std::uint64_t seed) const = 0;
    virtual std::vector<std::uint8_t> decapsulate(const std::vector<std::uint8_t>& ciphertext,
                                                  const std::vector<std::uint8_t>& privateKey) const = 0;
    virtual KemSizes sizes() const = 0;
};

}  // namespace v2x::crypto
