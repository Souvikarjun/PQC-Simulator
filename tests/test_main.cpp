#include "crypto/signature_scheme.hpp"
#include "crypto/kem_scheme.hpp"
#include "metrics/metrics.hpp"
#include "network/message.hpp"
#include "simulation/simulator.hpp"

#include <cassert>
#include <iostream>

int main() {
    auto scheme = v2x::crypto::createSignatureScheme("ML-DSA-44");
    const auto keys = scheme->generateKeyPair(42);
    const std::string message = "signed BSM";
    const auto signature = scheme->sign(message, keys);
    assert(scheme->verify(message, signature, keys.publicKey));
    assert(!scheme->verify("tampered BSM", signature, keys.publicKey));

    auto falcon = v2x::crypto::createSignatureScheme("FALCON-512");
    const auto falconKeys = falcon->generateKeyPair(42);
    const auto falconSignature = falcon->sign(message, falconKeys);
    assert(falcon->verify(message, falconSignature, falconKeys.publicKey));

    auto kem = v2x::crypto::createKemScheme("ML-KEM-512");
    const auto kemKeys = kem->generateKeyPair(42);
    const auto exchange = kem->encapsulate(kemKeys.publicKey, 42);
    assert(kem->decapsulate(exchange.ciphertext, kemKeys.privateKey) == exchange.sharedSecret);

    const auto statistics = v2x::metrics::summarize({1.0, 2.0, 3.0, 4.0});
    assert(statistics.count == 4);
    assert(statistics.mean == 2.5);
    assert(statistics.median == 2.5);

    v2x::simulation::Configuration config;
    config.vehicles = 3;
    config.durationSeconds = 0.2;
    config.communicationRangeMeters = 2000.0;
    config.packetLoss = 0.0;
    const auto result = v2x::simulation::Simulator(config).run();
    assert(result.metrics.generated == 6);
    assert(result.metrics.delivered == 18);
    assert(result.metrics.rsuDelivered == 6);
    assert(result.metrics.verified == 18);
    assert(result.metrics.rejected == 0);

    std::cout << "All simulator tests passed\n";
}
