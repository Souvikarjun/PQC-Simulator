#include "crypto/signature_scheme.hpp"

#include <algorithm>
#include <stdexcept>

namespace v2x::crypto {
namespace {

struct Profile {
    const char* name;
    SignatureSizes sizes;
    CryptoTiming timing;
};

const Profile& profileFor(const std::string& name) {
    static const Profile profiles[] = {
        {"ML-DSA-44", {1312, 2560, 2420}, {18.0, 105.0, 155.0}},
        {"ML-DSA-65", {1952, 4032, 3309}, {24.0, 145.0, 205.0}},
        {"ML-DSA-87", {2592, 4896, 4627}, {30.0, 190.0, 260.0}},
        {"SLH-DSA-SHA2-128s", {32, 64, 7856}, {35.0, 1850.0, 410.0}},
        {"Ed25519", {32, 64, 64}, {8.0, 32.0, 48.0}},
        {"ECDSA-P256", {65, 32, 72}, {40.0, 75.0, 110.0}},
        {"SHA-DSA", {32, 64, 64}, {10.0, 40.0, 58.0}},
        {"FALCON-512", {897, 1281, 666}, {14.0, 82.0, 96.0}},
        {"FALCON-1024", {1793, 2305, 1280}, {20.0, 120.0, 145.0}},
        {"ML-KEM", {800, 1632, 768}, {10.0, 50.0, 70.0}},
        {"ML-KEM-512", {800, 1632, 768}, {10.0, 50.0, 70.0}},
        {"ML-KEM-768", {1184, 2400, 1088}, {14.0, 68.0, 90.0}},
        {"ML-KEM-1024", {1568, 3168, 1568}, {18.0, 86.0, 115.0}},
    };
    for (const auto& profile : profiles) {
        if (profile.name == name) return profile;
    }
    throw std::invalid_argument("unsupported signature scheme: " + name);
}

std::uint64_t digest(const std::string& message, std::uint64_t key) {
    std::uint64_t state = 1469598103934665603ULL ^ key;
    for (unsigned char byte : message) {
        state ^= byte;
        state *= 1099511628211ULL;
        state ^= state >> 29;
    }
    return state ^ (key * 0x9e3779b97f4a7c15ULL);
}

class ModeledSignatureScheme final : public SignatureScheme {
public:
    explicit ModeledSignatureScheme(std::string name) : name_(std::move(name)), profile_(profileFor(name_)) {}

    std::string name() const override { return name_; }
    SignatureKeyPair generateKeyPair(std::uint64_t seed) const override { return {seed, seed}; }

    Signature sign(const std::string& message, const SignatureKeyPair& keyPair) const override {
        const auto value = digest(message, keyPair.privateKey);
        Signature signature;
        signature.bytes.resize(profile_.sizes.signatureBytes);
        for (std::size_t index = 0; index < signature.bytes.size(); ++index) {
            signature.bytes[index] = static_cast<std::uint8_t>((value >> ((index % 8) * 8)) & 0xffU);
        }
        return signature;
    }

    bool verify(const std::string& message, const Signature& signature,
                std::uint64_t publicKey) const override {
        if (signature.bytes.size() != profile_.sizes.signatureBytes) return false;
        const auto value = digest(message, publicKey);
        for (std::size_t index = 0; index < 8 && index < signature.bytes.size(); ++index) {
            if (signature.bytes[index] != static_cast<std::uint8_t>((value >> (index * 8)) & 0xffU)) return false;
        }
        return true;
    }

    SignatureSizes sizes() const override { return profile_.sizes; }
    CryptoTiming timing() const override { return profile_.timing; }

private:
    std::string name_;
    Profile profile_;
};

}  // namespace

std::unique_ptr<SignatureScheme> createSignatureScheme(const std::string& name) {
    return std::make_unique<ModeledSignatureScheme>(name);
}

std::vector<std::string> supportedSignatureSchemes() {
    return {"ML-DSA-44", "ML-DSA-65", "ML-DSA-87", "SLH-DSA-SHA2-128s",
            "Ed25519", "ECDSA-P256", "SHA-DSA", "FALCON-512", "FALCON-1024",
            "ML-KEM", "ML-KEM-512", "ML-KEM-768", "ML-KEM-1024"};
}

}  // namespace v2x::crypto
