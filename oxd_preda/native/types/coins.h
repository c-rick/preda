#pragma once
#include "big_num.h"


namespace rvm
{
class Coins;
class CoinsMutable;
class NonFungibleToken;
class NonFungibleVault;
class NonFungibleVaultMutable;

namespace _details
{
	extern void	_AppendCoinSymbol(TokenId id, rt::String& append);

	template <typename BN>
	void __CoinJsonify(TokenId id, const BN &a, rt::Json &append)
	{
		a.ToString(append.ScopeWritingStringEscaped());
		auto s = append.ScopeWritingStringEscaped();
		s.String() += rt::SS(":");
		_AppendCoinSymbol(id, s.String());
	}

	template <typename TCOIN, TokenId default_coinid = TokenIdInvalid>
	void _CoinJsonify(const TCOIN &c, rt::Json &append)
	{
		if(c.GetId() != TokenIdInvalid)
		{
			__CoinJsonify(c.GetId(), c.GetAmount(), append);
		}
		else if(default_coinid != TokenIdInvalid)
		{
			auto s = append.ScopeWritingStringEscaped();
			s.String() += rt::SS("0:");
			_AppendCoinSymbol(default_coinid, s.String());
		}
		else
			append.Null();
	}
} // namespace _details

// representing fungible tokens
#pragma pack(push, 1)
class Coins
{
	TYPETRAITS_DECLARE_NON_POD;
	RVM_IMMUTABLE_TYPE(Coins)

	friend class Shard;
	friend class CoinsMutable;

protected:
	TokenId CoinId; // what coin it is
	BigNum Amount;		 // MOVE semantics, never be copied

public:
	Coins(decltype(0) x) : Amount(0)
	{
		ASSERT(x == 0);
		CoinId = TokenIdInvalid;
	}
	TokenId GetId() const { return CoinId; }
	auto &GetAmount() const { return Amount; }
	bool IsZero() const { return CoinId == TokenIdInvalid || Amount.IsZero(); }

	template <TokenId default_coinid>
	void Jsonify(rt::Json &append) const { _details::_CoinJsonify<Coins, default_coinid>(*this, append); }
	void Jsonify(rt::Json &append) const { _details::_CoinJsonify(*this, append); }
	UINT GetEmbeddedSize() const { return sizeof(TokenId) + Amount.GetEmbeddedSize(); }
	template <typename T_Coin>
	static UINT GetEmbeddedSize(const T_Coin &c) { return sizeof(TokenId) + BigNum::GetEmbeddedSize(c.Amount); }
	static UINT GetEmbeddedSizeByAmount(const BigNumRef &c) { return sizeof(TokenId) + BigNum::GetEmbeddedSize(c); }
	template <typename T_Coin>
	static bool IsEmbeddable(const T_Coin &c){ return BigNum::IsEmbeddable(c.Amount); }
	static bool IsEmbeddableByAmount(const BigNumRef &c){ return BigNum::IsEmbeddable(c); }
	template <typename T_Coin>
	UINT Embed(const T_Coin &c)
	{
		CoinId = c.CoinId;
		UINT ret = Amount.Embed(c.Amount) + sizeof(TokenId);
		rt::_CastToNonconst(&c)->CoinId = TokenIdInvalid; // clear to zero after embedding state out
		return ret;
	}
	void Withdraw(CoinsMutable &get, CoinsMutable &residue) const; // residue = this - get
	bool IsCoreCoin() const { return Amount.IsPositive() && CoinId == TokenIdCoreCoin; }
	static auto &Zero()
	{
		static const Coins _zero(0);
		return _zero;
	}
	static void GetTypeSignature(rt::String &n) { n += rt::SS("token"); }
};
#pragma pack(pop)

class CoinsMutable
{
	friend class Coins;
	friend class Shard;
	friend class SchedulingExecutionContext;

protected:
	TokenId CoinId;  // what coin it is
	BigNumMutable Amount; // MOVE semantics, never be copied

protected:
	void _AlignCoinId(CoinsMutable &x)
	{
		if(x.CoinId != TokenIdInvalid)
		{
			ASSERT(x.CoinId == CoinId);
		}
		else
			x.CoinId = CoinId;
	}

public:
	void Empty() { CoinId = TokenIdInvalid; }
	bool IsZero() const { return CoinId == TokenIdInvalid || Amount.IsZero(); }
	auto GetId() const { return CoinId; }
	void SetId(TokenId id){ CoinId = id; }
	auto &GetAmount() const { return Amount; }
	auto GetModifiableAmount() -> BigNumMutable &; // CoinId will be set to zero

	CoinsMutable() { CoinId = TokenIdInvalid; }
	CoinsMutable(TokenId coin_id) : CoinId(coin_id) {}
	CoinsMutable(const CoinsMutable &x) = delete;
	CoinsMutable(CoinsMutable &&x) : CoinId(x.GetId()) { rt::Swap(Amount, x.Amount); }
	~CoinsMutable() { ASSERT(IsZero()); }

	void Deposit(const Coins &a); // new_total = this + to_merge

	void TransferTo(CoinsMutable &x);
	void TransferTo(CoinsMutable &x, const BigNumRef &b);
	void Merge(const Coins &a, const Coins &b);	 // new_total = this + to_merge
	void Merge(CoinsMutable &a, const Coins &b); // new_total = this + to_merge
	void Mint(TokenId aid, const BigNumRef &amount);
	bool FromString(const rt::String_Ref &str);
	bool JsonParse(const rt::String_Ref &str) { return FromString(str.TrimQuotes()); }

	template <TokenId default_coinid>
	void Jsonify(rt::Json &append) const { _details::_CoinJsonify<CoinsMutable, default_coinid>(*this, append); }
	void Jsonify(rt::Json &append) const { _details::_CoinJsonify(*this, append); }
};

namespace _details
{
	extern void SetCoinId(CoinsMutable &cm, TokenId coin_id);					// internal use only
	extern void SetCoins(Coins &cm, TokenId coin_id, const BigNumMutable &a);	// internal use only
}																				// namespace _details

#pragma pack(push, 1)
class NonFungibleToken // TXN ARG only
{
	friend class NonFungibleVault;
	friend class NonFungibleVaultMutable;

	NonfungibleId NonFungible;
	TYPETRAITS_DECLARE_POD;

public:
	NonFungibleToken(decltype(0) x)
	{
		ASSERT(x == 0);
		NonFungible = NonfungibleIdZero;
	}
	auto GetId() const { return NonFungible; }
	bool IsZero() const { return NonFungible == NonfungibleIdZero; }
	static auto &Zero()
	{
		static const NonfungibleId _zero(NonfungibleIdZero);
		return (const NonFungibleToken &)_zero;
	}
	static void GetTypeSignature(rt::String &n) { n += rt::SS("nonfungible"); }
};

class NonFungibleTokenRanged // TXN ARG only
{
	friend class NonFungibleVault;
	friend class NonFungibleVaultMutable;

	NonfungibleId	NonFungibleStart;
	UINT				Count;
	TYPETRAITS_DECLARE_POD;

public:
	NonFungibleTokenRanged(NonfungibleId s, UINT c):NonFungibleStart(s),Count(c){}
	NonFungibleTokenRanged(decltype(0) x)
	{
		ASSERT(x == 0);
		NonFungibleStart = NonfungibleIdZero;
		Count = 0;
	}
	bool IsZero() const { return Count == 0 || NonFungibleStart == NonfungibleIdZero; }
	static auto &Zero()
	{
		static const rt::_details::Zeros<sizeof(NonFungibleTokenRanged)> _;
		return (const NonFungibleTokenRanged &)_;
	}
	static void GetTypeSignature(rt::String &n) { n += rt::SS("nonfungible_series"); }
};

// class NonFungibleTokenSet
// {
// 	NonFungibleVaultMutable NonFungibleSet;
// 	TYPETRAITS_DECLARE_NON_POD;

// public:
// 	NonFungibleToken(decltype(0) x)
// 	{
// 		ASSERT(x == 0);
// 	}
// 	auto GetId() const
// 	{
// 		return NonFungibleSet;
// 	}
// 	bool IsZero() const
// 	{
// 		return true
// 	}
// 	// static auto&	Zero(){

// 	// }
// 	static void GetTypeSignature(rt::String &n)
// 	{
// 		n += rt::SS("nonfungible set");
// 	}
// 	bool Set(const NonFungibleVault &idSet)
// 	{
// 		/ NonFungibleSet.Assign(idSet);
// 		return true;
// 	}
// };
#pragma pack(pop)

} // namespace rvm