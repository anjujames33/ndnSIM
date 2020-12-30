/*
 * Copyright ( C ) 2020 New Mexico State University
 *
 * George Torres, Anju Kunnumpurathu James
 * See AUTHORS.md for complete list of authors and contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * ( at your option ) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NDN_CONSUMER_SYNC_H
#define NDN_CONSUMER_SYNC_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"

#include <set>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include "nlohmann/json.hpp"

namespace ns3 {
namespace ndn {

/**
 * \ingroup ndn
 * \defgroup ndnQoS ndnQoS apps and classes
 */

/**
 * @ingroup ndnQoS
 * \brief NDN application meant to be used with experimental project PowerSys-Cosim.
 *
 * This app injects payloaded Interest packets at node location. 
 * Does not schedule any packets on its own, completely relies on ndn-synchronizer 
 * class for scehduling packets and defining payloads.
 */
class ConsumerSync : public App {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor.
   * Sets up randomizer function and packet sequence number.
   */
  ConsumerSync();
  virtual ~ConsumerSync();

  // From App
  virtual void
  OnData(shared_ptr<const Data> contentObject);
 
  /**
   * @brief Timeout event.
   * @param sequenceNumber time outed sequence number.
   */
  virtual void
  OnTimeout(uint32_t sequenceNumber);

  /**
   * @brief Creates and sends out packet as described by parameters.
   * \param deviceName Name to be used as prefix for interest.
   * \param payload The data that will be attached to the interest.
   * \param agg If true the payload is appended to a json object that
   * aggregates said payload with previous ones, and combines them into
   * one large payload for sending.
   */
  void
  SendPacket(std::string deviceName, std::string payload, bool agg);
   /**
   * @brief An event that is fired just before an Interest packet is actually send out (send is
   *inevitable)
   *
   * The reason for "before" even is that in certain cases (when it is possible to satisfy from the
   *local cache),
   * the send call will immediately return data, and if "after" even was used, this after would be
   *called after
   * all processing of incoming data, potentially producing unexpected results.
   */
  virtual void
  WillSendOutInterest(uint32_t sequenceNumber);

  /**
   * @brief Schedules packets for sending, passing on relevent parameters.
   * \param deviceName Name to be used as prefix for interest.
   * \param payload The data that will be attached to the interest.
   * \param agg If true the payload is appended to a json object that
   * aggregates said payload with previous ones, and combines them into 
   * one large payload for sending.
   */
  void
  ScheduleNextPacket(std::string deviceName, std::string payload, bool agg);

    /**
   * @brief Aggregates payloads from follower DERs.
   *
   * Only Used if node in question is currently a LeadDER. Payloads from 
   * followers are appended to our orginal payload.
   */

  void
  updateLeadMeasurements(std::string newM, std::string device){
     leadMeasurements[device] = newM;
  }

  /**
   * @brief Node is set as a Lead DER, and resets payload.
   */

  void
  setAsLead(std::string Name){
     resetLead();
     auto it = m_payloads.begin();
     while(it != m_payloads.end()){
        leadMeasurements[it->first]=it->second;
	it++;
     }
  }

    /**
   * @brief Removes lead statues from node and resets payload.
   */
  void
  resetLead(){
     leadMeasurements.clear();
  }

public:
  //typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);

  typedef void (*SentInterestTraceCallback)( uint32_t, shared_ptr<const Interest> );
  typedef void (*ReceivedDataTraceCallback)( uint32_t, shared_ptr<const Data> );

protected:

  // From App
  virtual void
  StartApplication();

  virtual void
  StopApplication();

  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */

  /**
   * \brief Checks if the packet need to be retransmitted becuase of retransmission timer expiration
   */
  void
  CheckRetxTimeout();

  /**
   * \brief Modifies the frequency of checking the retransmission timeouts
   * \param retxTimer Timeout defining how frequent retransmission timeouts should be checked
   */
  void
  SetRetxTimer(Time retxTimer);

  /**
   * \brief Returns the frequency of checking the retransmission timeouts
   * \return Timeout defining how frequent retransmission timeouts should be checked
   */
  Time
  GetRetxTimer() const;

protected:
  Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator
  uint32_t m_seq;      ///< @brief currently requested sequence number
  uint32_t m_seqMax;   ///< @brief maximum number of sequence number
  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
  Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
  EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed

  Time m_txInterval;
  Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
  Time m_interestLifeTime; ///< \brief LifeTime for interest packet
  bool m_firstTime;
  uint32_t m_subscription; //subscription value set by the application
  uint32_t m_virtualPayloadSize; //payload size for interest packet
  uint32_t m_doRetransmission; //retransmit lost interest packets if set to 1
  uint32_t m_offset; //random offset

  nlohmann::json leadMeasurements; ///< @brief Json object that aggreates all recieved payloads for Lead DERs.
  std::unordered_map<std::string, std::string> m_payloads; ///< @brief Maps payloads to followers in order to prevent the inclusion of multiple  measurements for the same device. 
  Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator
  //int packetsToSend = 3;

  /// @cond include_hidden
  /**
   * \struct This struct contains sequence numbers of packets to be retransmitted
   */
  struct RetxSeqsContainer : public std::set<uint32_t> {
  };

  RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted

  /**
   * \struct This struct contains a pair of packet sequence number and its timeout
   */
  struct SeqTimeout {
    SeqTimeout(uint32_t _seq, Time _time)
      : seq(_seq)
      , time(_time)
    {
    }

    uint32_t seq;
    Time time;
  };
  /// @endcond

  /// @cond include_hidden
  class i_seq {
  };
  class i_timestamp {
  };
  /// @endcond

  /// @cond include_hidden
   /**
   * \struct This struct contains a multi-index for the set of SeqTimeout structs
   */
  struct SeqTimeoutsContainer
    : public boost::multi_index::
        multi_index_container<SeqTimeout,
                              boost::multi_index::
                                indexed_by<boost::multi_index::
                                             ordered_unique<boost::multi_index::tag<i_seq>,
                                                            boost::multi_index::
                                                              member<SeqTimeout, uint32_t,
                                                                     &SeqTimeout::seq>>,
                                           boost::multi_index::
                                             ordered_non_unique<boost::multi_index::
                                                                  tag<i_timestamp>,
                                                                boost::multi_index::
                                                                  member<SeqTimeout, Time,
                                                                         &SeqTimeout::time>>>> {
  };
  /// @endcond

  SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

  SeqTimeoutsContainer m_seqLastDelay;
  SeqTimeoutsContainer m_seqFullDelay;

  std::map<uint32_t, uint32_t> m_seqRetxCounts;

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
    uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;
  TracedCallback < uint32_t, shared_ptr<const Interest> > m_sentInterest;
  TracedCallback < uint32_t, shared_ptr<const Data> > m_receivedData;

};

} // namespace ndn
} // namespace ns3

#endif
