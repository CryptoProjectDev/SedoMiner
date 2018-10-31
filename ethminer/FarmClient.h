/**
* This file is generated by jsonrpcstub, DO NOT CHANGE IT MANUALLY!
*/

#ifndef JSONRPC_CPP_STUB_FARMCLIENT_H_
#define JSONRPC_CPP_STUB_FARMCLIENT_H_

#include <jsonrpccpp/client.h>
#include <ethminer/SignTx.h>
#include <libethash/sha3_cryptopp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>

#ifndef _WIN32
#define stricmp strcasecmp
#endif

using namespace std;
using namespace dev;
using namespace dev::eth;


class FarmClient : public jsonrpc::Client
{
public:

	enum TxStatus
	{
		Succeeded,
		Failed,
		Waiting,
		NotFound
	};

	class CMiner {
	public:
		string txHash;
		string account;
		int gasPrice;
		bytes challenge;

	};

	// Constructor
	FarmClient(
		jsonrpc::IClientConnector &conn,
		OperationMode _opMode,
		std::string _userAcct,
		jsonrpc::clientVersion_t type = jsonrpc::JSONRPC_CLIENT_V2
	) : jsonrpc::Client(conn, type)
	{
		m_opMode = _opMode;
		m_userAcct = _userAcct;
		
		m_tokenContract = ProgOpt::Get("SEDO", "TokenContract");
		
		if (_opMode == OperationMode::Solo)
		{
			m_userEthAddressPrivateKey = ProgOpt::Get("SEDO", "EthAddressPrivateKey");
			m_bidTop = 2;
			string s = ProgOpt::Get("SEDO", "BidTop", "2");
			if (isDigits(s))
				m_bidTop = atoi(s.c_str());
		}
	}

	void getWorkPool(bytes& _challenge, h256& _target, uint64_t& _difficulty, string& _hashingAcct)
	{
		jsonrpc::BatchCall batchCall = jsonrpc::BatchCall();

		Json::Value data;
		data.append(m_userAcct);

		// the first 2 calls don't need any parameter input, but there seems to be a bug in
		// the libjson-rpc-cpp package, such that it produces invalid JSON request if you just
		// do something like : batchCall.addCall("getChallengeNumber", Json::Value());
		// so I pass in some data which luckily the pool ignores, and sends me the information I want.

		int challengeID = batchCall.addCall("getChallengeNumber", data);
		int poolAddressID = batchCall.addCall("getPoolEthAddress", data);
		int targetID = batchCall.addCall("getMinimumShareTarget", data);
		int difficultyID = batchCall.addCall("getMinimumShareDifficulty", data);

		jsonrpc::BatchResponse response = CallProcedures(batchCall);

		if (response.hasErrors())
		{
			LogB << "Error in getWorkPool: JSON-RPC call resulted in errors";
		}

		_challenge = fromHex(response.getResult(challengeID).asString());
		m_target = u256(response.getResult(targetID).asString());
		m_hashingAcct = response.getResult(poolAddressID).asString();
		m_difficulty = atoll(response.getResult(difficultyID).asString().c_str());

		_target = m_target;
		_difficulty = m_difficulty;
		_hashingAcct = m_hashingAcct;
		LogF << "Trace: getWorkPool - challenge:" << toHex(_challenge).substr(0, 8)
			<< ", target:" << std::hex << std::setw(16) << std::setfill('0') << upper64OfHash(_target)
			<< ", difficulty:" << std::dec << _difficulty;
	}

	void getWorkPool_old(bytes& _challenge, h256& _target, uint64_t& _difficulty, string& _hashingAcct)
	{
		static Timer s_lastFetch;
		Json::Value data;
		Json::Value result;
		result = CallMethod("getChallengeNumber", data);
		_challenge = fromHex(result.asString());
		data.append(m_userAcct);
		result = CallMethod("getMinimumShareTarget", data);
		m_target = u256(result.asString());

		if (s_lastFetch.elapsedSeconds() > 20 || m_hashingAcct == "")
		{
			// no reason to retrieve this stuff every time.
			result = CallMethod("getPoolEthAddress", Json::Value());
			m_hashingAcct = result.asString();
			result = CallMethod("getMinimumShareDifficulty", data);
			m_difficulty = atoll(result.asString().c_str());
			s_lastFetch.restart();
		}

		_target = m_target;
		_difficulty = m_difficulty;
		_hashingAcct = m_hashingAcct;
		LogF << "Trace: getWorkPool - challenge:" << toHex(_challenge).substr(0, 8)
			<< ", target:" << std::hex << std::setw(16) << std::setfill('0') << upper64OfHash(_target)
			<< ", difficulty:" << std::dec << _difficulty;
	}

	void getWorkSolo(bytes& _challenge, h256& _target) throw (jsonrpc::JsonRpcException)
	{
		// challenge
		Json::Value p;
		p["from"] = m_userAcct;
		p["to"] = m_tokenContract;

		h256 bMethod = sha3("getChallengeNumber()");
		std::string sMethod = toHex(bMethod, dev::HexPrefix::Add);
		p["data"] = sMethod.substr(0, 10);

		Json::Value data;
		data.append(p);
		data.append("latest");

		Json::Value result = CallMethod("eth_call", data);
		if (result.isString())
		{
			_challenge = fromHex(result.asString());
		} else
			throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());
		LogF << "Trace: getWork, Challenge : " << toHex(_challenge);

		// target
		bMethod = sha3("getMiningTarget()");
		sMethod = toHex(bMethod, dev::HexPrefix::Add);
		p["data"] = sMethod.substr(0, 10);

		data.clear();
		data.append(p);
		data.append("latest");

		result = CallMethod("eth_call", data);
		if (result.isString())
		{
			_target = h256(result.asString());
		}
		else
			throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());

	}

	void submitWorkPool(h256 _nonce, bytes _hash, bytes _challenge, uint64_t _difficulty)
	{
		LogF << "Trace: SubmitWorkPool, challenge = " << toHex(_challenge);
		LogF << "Trace: SubmitWorkPool, nonce = " << _nonce;
		LogF << "Trace: SubmitWorkPool, digest = " << toHex(_hash);
		Json::Value data;
		data.append("0x" + _nonce.hex());
		data.append(devFeeMining ? DonationAddress : m_userAcct);
		data.append("0x" + toHex(_hash));
		data.append((Json::UInt64)_difficulty);
		data.append("0x" + toHex(_challenge));
		Json::Value result = CallMethod("submitShare", data);
		//if (!result.isString() || result.asString() != "ok")
		//	LogB << "Solution was rejected by the pool! Reason : " << result.asString();
		if (!result.isBool() || !result.asBool())
			LogB << "Solution was rejected by the pool!";

	}

	void submitWorkSolo(h256 _nonce, bytes _hash, bytes _challenge)
	{

		try
		{
			// check if any other miner in our farm already submitted a solution for this challenge
			string folder = ProgOpt::Get("SEDO", "ChallengeFolder");
			if (folder != "")
			{
				boost::filesystem::path m_challengeFilename = boost::filesystem::path(folder) / "challenge.txt";
				ifstream ifs;
				if (boost::filesystem::exists(m_challengeFilename))
				{
					string s;
					ifs.open(m_challengeFilename.generic_string(), fstream::in);
					getlineEx(ifs, s);
					if (s == toHex(_challenge))
					{
						LogS << "Another miner in the local farm already got this one : " << toHex(_challenge).substr(0, 8);
						return;
					}
				}
				ifs.close();
				// write this challenge value to our synchronization file.
				std::ofstream ofs(m_challengeFilename.generic_string(), std::ofstream::out);
				ofs << toHex(_challenge);
				ofs.close();
			}
		}
		catch (const std::exception& e)
		{
			LogB << "Exception: submitWorkSolo::CheckChallengeFolder - " << e.what();
		}


		// prepare transaction
		Transaction t;
		if (m_lastSolution.elapsedSeconds() > 5 * 60 || m_txNonce == -1)
			m_txNonce = getNextNonce();
		else
			m_txNonce++;
		m_lastSolution.restart();
		t.nonce = m_txNonce;
		t.receiveAddress = toAddress(m_tokenContract);
		ProgOpt::Reload();
		t.gas = atoi(ProgOpt::Get("SEDO", "GasLimit", "200000").c_str());
		m_startGas = atoi(ProgOpt::Get("SEDO", "GasPrice").c_str());
		m_maxGas = atoi(ProgOpt::Get("SEDO", "MaxGasPrice").c_str());
		t.gasPrice = RecommendedGasPrice(_challenge);

		// compute data parameter : first 4 bytes is hash of function signature
		h256 bMethod = sha3("mint(uint256,bytes32)");
		//h256 bMethod = sha3("proxyMint(uint256,bytes32)");
		std::string sMethod = toHex(bMethod, dev::HexPrefix::Add);
		sMethod = sMethod.substr(0, 10);
		// put the nonce in
		stringstream ss;
		ss << std::setw(64) << std::setfill('0') << _nonce.hex();
		std::string s2(ss.str());
		sMethod = sMethod + s2;
		// and the hash
		ss = std::stringstream();
		ss << std::left << std::setw(64) << std::setfill('0') << toHex(_hash);
		s2 = std::string(ss.str());
		sMethod = sMethod + s2;
		t.data = fromHex(sMethod);
		t.value = 0;
		t.challenge = _challenge;

		txSignSend(t);
		LogB << "Tx hash : " << t.txHash << ", gasPrice : " << t.gasPrice / 1000000000;
		m_pendingTxs.push_back(t);
	}

	void setChallenge(bytes& _challenge)
	{
		m_recentChallenges.push_front(_challenge);
		if (m_recentChallenges.size() > 4)
			m_recentChallenges.pop_back();
	}

	// this routine runs on a separate thread
	void txpoolScanner()
	{
		Json::Value data;
		Json::Value result;
		int sleepTime = 10000;

		if (tx_filterID == "")
		{
			// sign up for pending transactions
			try
			{
				data.clear();
				result = CallMethod("eth_newPendingTransactionFilter", data);
				tx_filterID = result.asString();
				if (tx_filterID == "")
					LogS << "Unable to initialize txpool filtering.  Your node may not support it. Try disabling GasPriceBidding.";
				else
					sleepTime = 1000;
			}
			catch (std::exception& e)
			{
				LogB << "Error calling eth_newPendingTransactionFilter" << e.what();
				LogB << "Your node may not support txpool filtering. Try disabling GasPriceBidding.";
				tx_filterID = "";
			}
		}

		// check existing bidders to see if any need removal.
		for (int i = m_biddingMiners.size() - 1; i >= 0; i--)
		{
			CMiner miner = m_biddingMiners[i];
			try
			{
				// check to see if the tx they submitted is still there
				Json::Value tx;
				data.clear();
				data.append(miner.txHash);
				tx = CallMethod("eth_getTransactionByHash", data);
				if (tx.isNull())
				{
					LogD << "Miner " << miner.account.substr(0, 10) << " is no longer in the txpool. tx = " << miner.txHash.substr(0, 10);
					m_biddingMiners.erase(m_biddingMiners.begin() + i);
				} else
				{
					// check for the existence of a tx receipt, which indicates the tx got mined.
					data.clear();
					data.append(miner.txHash);
					try
					{
						tx = CallMethod("eth_getTransactionReceipt", data);
						if (!tx.isNull())
						{
							string bidStatus = tx["status"].asString() == "0x1" ? " won" : (tx["status"].asString() == "0x0" ? " lost" : " ???");
							LogD << "Miner " << miner.account.substr(0, 10) << bidStatus << ", block: "
								<< HexToInt(tx["blockNumber"].asString()) << ", tx=" << miner.txHash.substr(0, 10);
							m_biddingMiners.erase(m_biddingMiners.begin() + i);
						}
					}
					catch (...)
					{
						// most likely transaction receipt not found
					}
				}
			}
			catch (std::exception& e)
			{
				LogB << "Error calling eth_getTransactionByHash" << e.what();
				result.clear();
			}
		}


		// request the hashes of all txs received since the last time we called.
		try
		{
			if (tx_filterID == "")
				result.clear();
			else
			{
				data = Json::Value();
				data.append(tx_filterID);
				result = CallMethod("eth_getFilterChanges", data);
			}
		}
		catch (std::exception& e)
		{
			LogB << "Error calling eth_getFilterChanges" << e.what();
			result.clear();
		}

		// we've only got the hashes at this point,  so now retrieve each tx
		for (uint32_t i = 0; i < result.size(); i++)
		{
			string hash = result[i].asString();
			data.clear();
			data.append(hash);
			try {
				Json::Value tx = CallMethod("eth_getTransactionByHash", data);
				if (!tx.isNull()) {
					string toAddr = tx["to"].asString();
					string input = tx["input"].asString();
					// look for txs addressed to the SEDO contract that are calling the mint() function
					if (LowerCase(toAddr) == m_tokenContract && input.substr(0, 10) == "0x1801fbe5") {
					//if (LowerCase(toAddr) == m_tokenContract && input.substr(0, 10) == "b1bb4d35") {
						CMiner miner;
						miner.txHash = hash;
						miner.account = tx["from"].asString();
						miner.gasPrice = HexToInt(tx["gasPrice"].asString()) / 1000000000;
						// try to determine what challenge this miner is submitting for.
						// pull out the nonce and the hash
						h256 nonce = h256(input.substr(10, 64));
						bytes minerhash = fromHex(input.substr(74, 64));
						h160 sender = h160(miner.account);
						bytes hash(32);
						{
							for (auto c : m_recentChallenges)
							{
								keccak256_SEDO(c, sender, nonce, hash);
								if (hash == minerhash)
								{
									miner.challenge = c;
								}
							}
						}
						LogD << "Miner " << miner.account.substr(0, 10) << " submitted tx " << miner.txHash.substr(0, 10)
							<< ". gasPrice=" << miner.gasPrice << ", challenge=" << toHex(miner.challenge).substr(0, 8);
						m_biddingMiners.push_back(miner);
					}
				}
			}
			catch (std::exception& e) {
				LogB << "Error calling eth_getTransactionByHash " << e.what();
			}
		}

	}

	void closeTxFilter() {
		if (tx_filterID == "") return;
		// cancel the filter
		Json::Value data;
		data.append(tx_filterID);
		CallMethod("eth_uninstallFilter", data);
	}

	int getNextNonce() {
		// get transaction count for nonce
		Json::Value p;
		p.append(m_userAcct);
		p.append("latest");
		Json::Value result = CallMethod("eth_getTransactionCount", p);
		return HexToInt(result.asString());
	}

	uint64_t tokenBalance() {
		Json::Value p;
		p["from"] = m_userAcct;
		p["to"] = m_tokenContract;

		h256 bMethod = sha3("balanceOf(address)");
		std::string sMethod = toHex(bMethod, dev::HexPrefix::Add);
		sMethod = sMethod.substr(0, 10);

		// address
		stringstream ss;
		ss << std::setw(64) << std::setfill('0') << m_userAcct.substr(2);
		std::string s2(ss.str());
		sMethod = sMethod + s2;
		p["data"] = sMethod;

		Json::Value data;
		data.append(p);
		data.append("latest");

		try
		{
			Json::Value result = CallMethod("eth_call", data);
			u256 balance = u256(result.asString()) / 100000000;
			return static_cast<uint64_t>(balance);
		}
		catch (std::exception& e)
		{
			LogD << "Error in routine tokenBalance: " << e.what();
			return 0;
		}
	}


	TxStatus getTxStatus(string _txHash)
	{
		Json::Value data;
		data.append(_txHash);
		try
		{
			// check if the tx still exists
			Json::Value result = CallMethod("eth_getTransactionByHash", data);
			if (result.isNull())
				return TxStatus::NotFound;

			// check if the tx has been mined
			result = CallMethod("eth_getTransactionReceipt", data);
			if (result["status"].asString() == "0x1")
				return TxStatus::Succeeded;
			else if (result["status"].asString() == "0x0")
				return TxStatus::Failed;
			else
				return TxStatus::Waiting;
		}
		catch (...)
		{
			return TxStatus::Waiting;
		}
	}

	u256 RecommendedGasPrice(bytes _challenge)
	{
		u256 recommendation = 0;
		for (auto m : m_biddingMiners)
		{
			if (m.challenge == _challenge && stricmp(m.account.c_str(), m_userAcct.c_str()) != 0)
			{
				LogF << "Trace: RecommendedGasPrice, existing bidder " << m.account.substr(0,10) << ", gasPrice=" << m.gasPrice;
				if (m.gasPrice > recommendation)
					recommendation = m.gasPrice;
			}
		}

		// can't get the max macro to work ... stupid conflicts!
		if (recommendation == 0)
			recommendation = m_startGas;
		else
			recommendation += m_bidTop;

		if (recommendation < m_startGas)
			recommendation = m_startGas;

		if (recommendation > m_maxGas)
			recommendation = m_startGas;

		return recommendation * 1000000000;
	}

	void checkPendingTransactions()
	{
		vector<bytes> needsDeleting;

		for (int i = m_pendingTxs.size() - 1; i >= 0; i--)
		{
			Transaction t = m_pendingTxs[i];
			TxStatus status = getTxStatus(t.txHash);
			if (status == Succeeded)
			{
				needsDeleting.push_back(t.challenge);
				LogB << "Tx " << t.txHash.substr(0, 10) << " succeeded, gasPrice = " << t.gasPrice / 1000000000 << "  :)";
			}
			else if (status == Failed)
			{
				needsDeleting.push_back(t.challenge);
				LogB << "Tx " << t.txHash.substr(0, 10) << " failed, gasPrice = " << t.gasPrice / 1000000000 << "  :(";
			}
			else if (status == Waiting)
			{
				// adjust gas price if necessary
				u256 recommended = RecommendedGasPrice(t.challenge);
				if (t.gasPrice < recommended)
				{
					// increase gas price and resend
					// but first make a copy of existing tx.  we can't be sure if the old one will get replaced
					// by the new one, before one or the other gets mined.
					t.gasPrice = recommended;
					txSignSend(t);
					m_pendingTxs.push_back(t);
					LogB << "Adjusting gas price to " << t.gasPrice / 1000000000 << ", new tx hash=" << t.txHash;
				}
			}
		}

		for (auto challenge : needsDeleting)
		{
			for (int i = m_pendingTxs.size() - 1; i >= 0; i--)
			{
				if (m_pendingTxs[i].challenge == challenge)
					m_pendingTxs.erase(m_pendingTxs.begin() + i);
			}
		}
	}

	void txSignSend(Transaction &t)
	{
		stringstream ss;
		Secret pk = Secret(m_userEthAddressPrivateKey);
		t.sign(pk);
		ss = std::stringstream();
		ss << "0x" << toHex(t.rlp());

		// submit to the node
		Json::Value p;
		p.append(ss.str());
		Json::Value result = CallMethod("eth_sendRawTransaction", p);
		t.txHash = result.asString();
	}



	void testHash(bytes _challenge, string _sender, h256 _nonce)  throw (jsonrpc::JsonRpcException) {
		std::vector<byte> mix(84);
		std::ostringstream ss;
		Json::Value p;
		p["from"] = _sender;
		p["to"] = m_tokenContract;        // SEDO contract address

		// function signature
		h256 bMethod = sha3("getMintDigest(uint256,bytes32,bytes32)");
		std::string sMethod = toHex(bMethod, dev::HexPrefix::Add);
		sMethod = sMethod.substr(0, 10);

		// nonce
		ss << std::setw(64) << std::setfill('0') << _nonce.hex();
		std::string s2(ss.str());
		sMethod = sMethod + s2;
		memcpy(&mix[52], _nonce.data(), 32);

		// challenge_digest. this is actually an unused parameter in the contract, so we
		// just send in the challenge_number twice.
		ss = std::ostringstream();
		ss << std::left << std::setw(64) << std::setfill('0') << toHex(_challenge);
		s2 = std::string(ss.str());
		sMethod = sMethod + s2;

		// challenge_number
		sMethod = sMethod + s2;

		p["data"] = sMethod;

		Json::Value data;
		data.append(p);
		data.append("latest");

		Json::Value result = this->CallMethod("eth_call", data);
		if (result.isString()) {
			LogS << "test hash";
			LogS << result.asString();
			h160 sender(_sender);
			memcpy(&mix[0], _challenge.data(), 32);
			memcpy(&mix[32], sender.data(), 20);
			bytes hash(32);
			keccak256_SEDO(_challenge, sender, _nonce, hash);
			LogS << "0x" << toHex(hash);
			SHA3_256((const ethash_h256_t*) hash.data(), (const uint8_t*) mix.data(), 84);
			LogS << "0x" << toHex(hash);
			LogS << "end test";

		} else
			throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE, result.toStyledString());
	}

	/*-----------------------------------------------------------------------------------
	* getBlockNumber
	*----------------------------------------------------------------------------------*/
	unsigned getBlockNumber()
	{
		try
		{
			Json::Value p;
			Json::Value result = CallMethod("eth_blockNumber", p);
			return jsToInt(result.asString());
		}
		catch (const std::exception& e)
		{
			LogB << "Exception in getBlockNumber - " << e.what();
			return 0;
		}
	}


public:
	bool devFeeMining = false;

private:
	OperationMode m_opMode;
	string m_userEthAddressPrivateKey;
	string m_tokenContract;
	int m_txNonce = -1;
	Timer m_lastSolution;
	vector<Transaction> m_pendingTxs;
	string tx_filterID;
	deque<bytes> m_recentChallenges;
	deque<CMiner> m_biddingMiners;
	int m_startGas;
	int m_maxGas;
	int m_bidTop;
	string m_userAcct;
	string m_hashingAcct = "";
	uint64_t m_difficulty;
	h256 m_target;

};

#endif //JSONRPC_CPP_STUB_FARMCLIENT_H_