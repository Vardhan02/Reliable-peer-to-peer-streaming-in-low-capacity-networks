#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/crypto++/aes.h"
#include "ns3/crypto++/modes.h"

using namespace ns3;

int main (int argc, char *argv[])
{
    //Initialize the simulation
    CommandLine cmd;
    cmd.Parse (argc,argv);

    //Create nodes
    NodeContainer nodes;
    nodes.Create (2);

    //Create point-to-point link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    //Install internet stack on nodes
    InternetStackHelper internet;
    internet.Install (nodes);

    //Create and configure a video sender application
    uint16_t port = 9;  //use port 9 for sending video
    Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (20.));

    //Create and configure a video source application
    OnOffHelper onOffHelper ("ns3::UdpSocketFactory", sinkAddress);
    onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onOffHelper.SetAttribute ("DataRate", StringValue ("50Mbps"));
    onOffHelper.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer sourceApps = onOffHelper.Install (nodes.Get (0));
    sourceApps.Start (Seconds (1.));
    sourceApps.Stop (Seconds (20.));

    //Create and configure an AES encryption object
    byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[CryptoPP::AES::BLOCKSIZE];
    memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH);
    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AES::Encryption aesEncryption(key, sizeof(key));
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

    //Encrypt the video data
    std::string plaintext = "Original video data";
    std::string ciphertext;
    CryptoPP::StreamTrans
