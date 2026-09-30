#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace v2x::crypto {

struct SignatureKeyPair {
    std::vector<std::uint8_t> privateKey;
    std::vector<std::uint8_t> publicKey;
};

struct Signature {
    std::vector<std::uint8_t> bytes;
};

struct SignatureSizes {
    std::size_t publicKeyBytes{};
    std::size_t privateKeyBytes{};
    std::size_t signatureBytes{};
};

class SignatureScheme {
public:
    virtual ~SignatureScheme() = default;
    virtual std::string name() const = 0;
    virtual SignatureKeyPair generateKeyPair(std::uint64_t seed) const = 0;
    virtual Signature sign(const std::string& message, const SignatureKeyPair& keyPair) const = 0;
    virtual bool verify(const std::string& message, const Signature& signature,
                        const std::vector<std::uint8_t>& publicKey) const = 0;
    virtual SignatureSizes sizes() const = 0;
};

std::unique_ptr<SignatureScheme> createSignatureScheme(const std::string& name);
std::vector<std::string> supportedSignatureSchemes();

}  // namespace v2x::crypto
