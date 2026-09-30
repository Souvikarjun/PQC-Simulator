#include "crypto/kem_scheme.hpp"

#include <oqs/oqs.h>

#include <array>
#include <mutex>
#include <stdexcept>

namespace v2x::crypto {
namespace {

constexpr std::array algorithms{
    OQS_KEM_alg_ml_kem_512,
    OQS_KEM_alg_ml_kem_768,
    OQS_KEM_alg_ml_kem_1024,
};

void initializeOqs() {
    static std::once_flag initialized;
    std::call_once(initialized, [] { OQS_init(); });
}

class LiboqsKemScheme final : public KemScheme {
public:
    explicit LiboqsKemScheme(std::string name) : name_(std::move(name)), kem_(nullptr, OQS_KEM_free) {
        initializeOqs();
        kem_.reset(OQS_KEM_new(name_.c_str()));
        if (!kem_) throw std::runtime_error("liboqs KEM algorithm is unavailable: " + name_);
    }

    std::string name() const override { return name_; }

    KemKeyPair generateKeyPair(std::uint64_t) const override {
        KemKeyPair keyPair;
        keyPair.publicKey.resize(kem_->length_public_key);
        keyPair.privateKey.resize(kem_->length_secret_key);
        if (OQS_KEM_keypair(kem_.get(), keyPair.publicKey.data(), keyPair.privateKey.data()) != OQS_SUCCESS) {
            throw std::runtime_error("liboqs KEM key generation failed");
        }
        return keyPair;
    }

    KemExchange encapsulate(const std::vector<std::uint8_t>& publicKey, std::uint64_t) const override {
        if (publicKey.size() != kem_->length_public_key) {
            throw std::invalid_argument("invalid liboqs KEM public key length");
        }
        KemExchange exchange;
        exchange.ciphertext.resize(kem_->length_ciphertext);
        exchange.sharedSecret.resize(kem_->length_shared_secret);
        if (OQS_KEM_encaps(kem_.get(), exchange.ciphertext.data(), exchange.sharedSecret.data(),
                           publicKey.data()) != OQS_SUCCESS) {
            throw std::runtime_error("liboqs KEM encapsulation failed");
        }
        return exchange;
    }

    std::vector<std::uint8_t> decapsulate(const std::vector<std::uint8_t>& ciphertext,
                                          const std::vector<std::uint8_t>& privateKey) const override {
        if (ciphertext.size() != kem_->length_ciphertext || privateKey.size() != kem_->length_secret_key) {
            throw std::invalid_argument("invalid liboqs KEM ciphertext or private key length");
        }
        std::vector<std::uint8_t> sharedSecret(kem_->length_shared_secret);
        if (OQS_KEM_decaps(kem_.get(), sharedSecret.data(), ciphertext.data(), privateKey.data()) != OQS_SUCCESS) {
            throw std::runtime_error("liboqs KEM decapsulation failed");
        }
        return sharedSecret;
    }

    KemSizes sizes() const override {
        return {kem_->length_public_key, kem_->length_secret_key, kem_->length_ciphertext,
                kem_->length_shared_secret};
    }

private:
    std::string name_;
    std::unique_ptr<OQS_KEM, decltype(&OQS_KEM_free)> kem_;
};

}  // namespace

std::unique_ptr<KemScheme> createKemScheme(const std::string& name) {
    for (const auto* algorithm : algorithms) {
        if (algorithm == name) return std::make_unique<LiboqsKemScheme>(name);
    }
    throw std::invalid_argument("unsupported KEM scheme: " + name);
}

std::vector<std::string> supportedKemSchemes() {
    initializeOqs();
    std::vector<std::string> supported;
    for (const auto* algorithm : algorithms) {
        if (OQS_KEM_alg_is_enabled(algorithm)) supported.emplace_back(algorithm);
    }
    return supported;
}

}  // namespace v2x::crypto