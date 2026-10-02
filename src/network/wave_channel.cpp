#include "network/wave_channel.hpp"

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/wifi-80211p-helper.h>
#include <ns3/yans-wifi-helper.h>

#include <cstring>
#include <stdexcept>

namespace v2x::network {

class WaveChannel::Impl {
public:
    Impl(std::size_t vehicleCount, double communicationRangeMeters)
        : vehicleCount_(vehicleCount) {
        nodes_.Create(static_cast<std::uint32_t>(vehicleCount + 1));

        ns3::MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(nodes_);

        ns3::YansWifiChannelHelper channel = ns3::YansWifiChannelHelper::Default();
        channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange",
                                  ns3::DoubleValue(communicationRangeMeters));
        ns3::YansWifiPhyHelper phy;
        phy.SetChannel(channel.Create());
        phy.Set("TxPowerStart", ns3::DoubleValue(20.0));
        phy.Set("TxPowerEnd", ns3::DoubleValue(20.0));

        const auto wifi = ns3::Wifi80211pHelper::Default();
        const auto mac = ns3::NqosWaveMacHelper::Default();
        devices_ = wifi.Install(phy, mac, nodes_);

        ns3::InternetStackHelper internet;
        internet.Install(nodes_);
        ns3::Ipv4AddressHelper addresses;
        addresses.SetBase("10.1.0.0", "255.255.0.0");
        addresses.Assign(devices_);

        for (std::uint32_t index = 0; index < nodes_.GetN(); ++index) {
                                        auto wifi = ns3::Wifi80211pHelper::Default();
                                        auto mac = ns3::NqosWaveMacHelper::Default();
            socket->SetAllowBroadcast(true);
            if (socket->Bind(ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port_)) != 0) {
                throw std::runtime_error("ns-3 WAVE receiver socket bind failed");
            }
            socket->SetRecvCallback(ns3::MakeCallback(&Impl::receive, this));
            sockets_.push_back(socket);
        }
    }

    ~Impl() { ns3::Simulator::Destroy(); }

    std::vector<WaveReception> broadcast(const std::vector<Position>& positions,
                                         const std::vector<WaveTransmission>& transmissions,
                                         double intervalSeconds) {
        if (positions.size() != vehicleCount_ + 1) {
            throw std::invalid_argument("ns-3 WAVE positions must include every vehicle and the RSU");
        }
        receptions_.clear();
        for (std::size_t index = 0; index < positions.size(); ++index) {
            nodes_.Get(static_cast<std::uint32_t>(index))
                ->GetObject<ns3::MobilityModel>()
                ->SetPosition(ns3::Vector(positions[index].x, positions[index].y, 0.0));
        }

        for (const auto& transmission : transmissions) {
            if (transmission.sender >= vehicleCount_) {
                throw std::invalid_argument("ns-3 WAVE transmission sender is not a vehicle");
            }
            std::vector<std::uint8_t> frame(sizeof(transmission.id) + transmission.payloadBytes);
            std::memcpy(frame.data(), &transmission.id, sizeof(transmission.id));
            auto packet = ns3::Create<ns3::Packet>(frame.data(),
                static_cast<std::uint32_t>(frame.size()));
            ns3::Simulator::Schedule(ns3::Seconds(transmission.offsetSeconds),
                &Impl::send, this, transmission.sender, packet);
        }

        ns3::Simulator::Stop(ns3::Seconds(intervalSeconds));
        ns3::Simulator::Run();
        return receptions_;
    }
            ns3::InetSocketAddress(ns3::Ipv4Address("10.1.255.255"), port_));
                                                &Impl::send, this, transmission.sender, transmission.id, transmission.payloadBytes);

    void receive(ns3::Ptr<ns3::Socket> socket) {
        ns3::Address source;
        while (auto packet = socket->RecvFrom(source)) {
            if (packet->GetSize() < sizeof(MessageId)) continue;
            MessageId message{};
            packet->CopyData(reinterpret_cast<std::uint8_t*>(&message), sizeof(message));
            const auto receiver = static_cast<NodeId>(socket->GetNode()->GetId());
            receptions_.push_back({message, receiver});
        }
    }

    static constexpr std::uint16_t port_ = 4242;
    std::size_t vehicleCount_;
    ns3::NodeContainer nodes_;
    ns3::NetDeviceContainer devices_;
    std::vector<ns3::Ptr<ns3::Socket>> sockets_;
    std::vector<WaveReception> receptions_;
};

WaveChannel::WaveChannel(std::size_t vehicleCount, double communicationRangeMeters)
    : impl_(std::make_unique<Impl>(vehicleCount, communicationRangeMeters)) {}
                                            if (packet->GetSize() < sizeof(MessageId) + sizeof(double)) continue;
WaveChannel::~WaveChannel() = default;
                                            double sendTime{};
                                            std::vector<std::uint8_t> header(sizeof(message) + sizeof(sendTime));
                                            packet->CopyData(header.data(), static_cast<std::uint32_t>(header.size()));
                                            std::memcpy(&message, header.data(), sizeof(message));
                                            std::memcpy(&sendTime, header.data() + sizeof(message), sizeof(sendTime));
std::vector<WaveReception> WaveChannel::broadcast(
                                            receptions_.push_back({message, receiver, ns3::Simulator::Now().GetSeconds() - sendTime});
    const std::vector<WaveTransmission>& transmissions,
    double intervalSeconds) {
    return impl_->broadcast(positions, transmissions, intervalSeconds);
}

}  // namespace v2x::network