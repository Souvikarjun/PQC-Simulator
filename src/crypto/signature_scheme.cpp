#include "crypto/signature_scheme.hpp"

#include <oqs/oqs.h>

#include <array>
#include <mutex>
#include <stdexcept>

namespace v2x::crypto {
namespace {

struct SignatureAlgorithm {
    const char* publicName;
    const char* oqsName;
};

constexpr std::array algorithms{
    SignatureAlgorithm{"ML-DSA-44", OQS_SIG_alg_ml_dsa_44},
    SignatureAlgorithm{"ML-DSA-65", OQS_SIG_alg_ml_dsa_65},
    SignatureAlgorithm{"ML-DSA-87", OQS_SIG_alg_ml_dsa_87},
    SignatureAlgorithm{"FALCON-512", OQS_SIG_alg_falcon_512},
    SignatureAlgorithm{"FALCON-1024", OQS_SIG_alg_falcon_1024},
    SignatureAlgorithm{"SPHINCS+-SHA2-128s-simple", OQS_SIG_alg_sphincs_sha2_128s_simple},
};

void initializeOqs() {
    static std::once_flag initialized;
    std::call_once(initialized, [] { OQS_init(); });
}

const SignatureAlgorithm& algorithmFor(const std::string& name) {
    for (const auto& algorithm : algorithms) {
        if (algorithm.publicName == name) {
            return algorithm;
        }
    }
    throw std::invalid_argument("unsupported signature scheme: " + name);
}

class LiboqsSignatureScheme final : public SignatureScheme {
public:
    explicit LiboqsSignatureScheme(std::string name)
        : algorithm_(algorithmFor(name)), signature_(nullptr, OQS_SIG_free) {
        initializeOqs();
        signature_.reset(OQS_SIG_new(algorithm_.oqsName));
        if (!signature_) {
            throw std::runtime_error("liboqs signature algorithm is unavailable: " + name);
        }
    }

    std::string name() const override { return algorithm_.publicName; }

    SignatureKeyPair generateKeyPair(std::uint64_t) const override {
        SignatureKeyPair keyPair;
        keyPair.publicKey.resize(signature_->length_public_key);
        keyPair.privateKey.resize(signature_->length_secret_key);
        if (OQS_SIG_keypair(signature_.get(), keyPair.publicKey.data(), keyPair.privateKey.data()) != OQS_SUCCESS) {
            throw std::runtime_error("liboqs signature key generation failed");
        }
        return keyPair;
    }

    Signature sign(const std::string& message, const SignatureKeyPair& keyPair) const override {
        Signature signature;
        signature.bytes.resize(signature_->length_signature);
        std::size_t signatureLength = 0;
        if (keyPair.privateKey.size() != signature_->length_secret_key ||
            OQS_SIG_sign(signature_.get(), signature.bytes.data(), &signatureLength,
                         reinterpret_cast<const std::uint8_t*>(message.data()), message.size(),
                         keyPair.privateKey.data()) != OQS_SUCCESS) {
            throw std::runtime_error("liboqs signature generation failed");
        }
        signature.bytes.resize(signatureLength);
        return signature;
    }

    bool verify(const std::string& message, const Signature& signature,
                const std::vector<std::uint8_t>& publicKey) const override {
        if (publicKey.size() != signature_->length_public_key || signature.bytes.empty()) return false;
        return OQS_SIG_verify(signature_.get(), reinterpret_cast<const std::uint8_t*>(message.data()),
                              message.size(), signature.bytes.data(), signature.bytes.size(),
                              publicKey.data()) == OQS_SUCCESS;
    }

    SignatureSizes sizes() const override {
        return {signature_->length_public_key, signature_->length_secret_key, signature_->length_signature};
    }

private:
    const SignatureAlgorithm& algorithm_;
    std::unique_ptr<OQS_SIG, decltype(&OQS_SIG_free)> signature_;
};

}  // namespace

std::unique_ptr<SignatureScheme> createSignatureScheme(const std::string& name) {
    return std::make_unique<LiboqsSignatureScheme>(name);
}

std::vector<std::string> supportedSignatureSchemes() {
    initializeOqs();
    std::vector<std::string> supported;
    for (const auto& algorithm : algorithms) {
        if (OQS_SIG_alg_is_enabled(algorithm.oqsName)) supported.emplace_back(algorithm.publicName);
    }
    return supported;
}

}  // namespace v2x::crypto
