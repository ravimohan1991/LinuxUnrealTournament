// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Array.h"
#include "UTUITypes.h"

namespace DPPrivate
{
	template <typename T>
	class TDataProviderInternal : public TSharedFromThis<TDataProviderInternal<T>>
	{
		typedef TArray<T> ArrayType;
		TDataProviderInternal& operator=( const TDataProviderInternal& ) = delete;

	public:
		/**
		* Register a listener for when this data provider is changed
		*/
		DECLARE_EVENT( TDataProviderInternal<T>, FOnChange );
		FOnChange& OnChange() { return OnChangeEvent; }

	public:
		TDataProviderInternal() = default;
		TDataProviderInternal( TDataProviderInternal&& Other )
		{
			Data          = Other.Data;
			OnChangeEvent = Other.OnChangeEvent;

			Other.Data.Empty();
			Other.OnChangeEvent.Clear();
		}

		template <typename U>
		TDataProviderInternal( typename TEnableIf<TIsDerivedFrom<U, T>::IsDerived, const TDataProviderInternal<U>&>::Type Other )
		{
			Data          = Other.Data;
			OnChangeEvent = Other.OnChangeEvent;
		}

		virtual ~TDataProviderInternal()
		{
			if ( TickHandle.IsValid() )
			{
				FTicker::GetCoreTicker().RemoveTicker( TickHandle );
				TickHandle.Reset();
			}
		}

		template <typename U>
		TDataProviderInternal& operator=( typename TEnableIf<TIsDerivedFrom<U, T>::IsDerived, const TDataProviderInternal<U>&>::Type Other )
		{
			if ( this == &Other )
			{
				return *this;
			}

			auto& Self         = *this;
			Self.Data          = Other.Data;
			Self.OnChangeEvent = Other.OnChangeEvent;
			return Self;
		}

		const TArray<T>& AsArray() const
		{
			return Data;
		}

		/**
		 * Filter this data provider returning an array of filtered results
		 *
		 * @param Fn method to use for filtering
		 * @return an array of elements matching Fn
		 */
		ArrayType Filter( TFunctionRef<bool( T )> Fn ) const
		{
			ArrayType Result;
			for ( auto& Iter : Data )
			{
				if ( Fn( Iter ) )
				{
					Result.Add( Iter );
				}
			}

			return Result;
		}

		/**
		 * Find an element in this data provider
		 *
		 * @param Fn method to use for finding
		 * @return optional found value
		 */
		TOptional<T> Find( TFunctionRef<bool( T )> Fn ) const
		{
			for ( const auto& Iter : Data )
			{
				if ( Fn( Iter ) )
				{
					return TOptional<T>( Iter );
				}
			}

			return TOptional<T>();
		}

		/**
		 * Remove elements from this data provider
		 *
		 * @param Fn method to use for determining elements to remove
		 * @return number of elements removed
		 */
		ArrayType Remove( TFunctionRef<bool( T )> Fn )
		{
			ArrayType Result;
			for ( int32 i = 0; i < Data.Num(); ++i )
			{
				if ( Fn( Data[ i ] ) )
				{
					Result.Add( Data[ i ] );
					Data.RemoveAt( i-- ); // this will call the remove function with the current value of i then decrement
					Invalidate();
				}
			}

			return Result;
		}

		/**
		 * Add a new value to this data provider
		 *
		 * @return a reference to the new item to be initialized
		 */
		T& Add()
		{
			Invalidate();
			int32 Index = Data.Emplace();
			return Data[ Index ];
		}

		/**
		 * Get the value at index
		 *
		 * @param Index
		 * @return
		 */
		T& At( int32 Index )
		{
			check( Data.IsValidIndex( Index ) );
			return Data[ Index ];
		}

		/**
		 * Get the value at index
		 *
		 * @param Index
		 * @return
		 */
		const T& At( int32 Index ) const
		{
			check( Data.IsValidIndex( Index ) );
			return Data[ Index ];
		}

		/** Bracket operator */
		T& operator[]( int32 Index ) { return Data[ Index ]; }

		/** const Bracket operator */
		const T& operator[]( int32 Index ) const { return Data[ Index ]; }

		/**
		 * Remove all entries from this data provider
		 *
		 * @return
		 */
		void Clear()
		{
			Invalidate();
			Data.Empty();
		}

		/**
		 * Append a variadic list of arguments to this data provider
		 *
		 * @param Values
		 * @return
		 */
		template <typename... TS>
		void Append( const TS&... Values )
		{
			Invalidate();
			Append_internal( Values... );
		}

		/**
		 * Set the content of this dataprovider from an array
		 *
		 * @param Values the array
		 */
		template <typename U = T, typename Allocator = FDefaultAllocator>
		void FromArray( const TArray<U, Allocator>& Values )
		{
			Invalidate();
			Data.Reset( Values.Num() );
			Data.Append( Values );
		}

		/** Data Length */
		int32 Num() const { return Data.Num(); }

		/**
		 * Count by predicate
		 * 
		 * @param Fn determine if entry counts
		 * @return number of counted entries
		 */
		int32 Num( TFunctionRef<bool( T )> Fn ) const
		{
			int32 Result = 0;
			for ( const auto& Iter : Data )
			{
				if ( Fn( Iter ) )
				{
					++Result;
				}
			}

			return Result;
		}

		/** Ensure Index is within the valid range for the data */
		bool IsValidIndex( int32 Index ) const { return Data.IsValidIndex( Index ); }

	protected:
		/**
		 * Register this data provider for tick so we can only update once
		 */
		void Invalidate()
		{
			if ( TickHandle.IsValid() )
			{
				return;
			}

			TickHandle =
			    FTicker::GetCoreTicker().AddTicker( FTickerDelegate::CreateLambda( [=]( float Delta ) -> bool { return OnUpdate( Delta ); } ) );
		}

		/**
		 * Called when this data provider ticks after being invalidated
		 */
		bool OnUpdate( float )
		{
			FTicker::GetCoreTicker().RemoveTicker( TickHandle );
			TickHandle.Reset();

			if ( TIsPointer<T>::Value )
			{
				Data.RemoveAllSwap( []( const T& Iter ) {
					if ( Iter == nullptr )
					{
						return true;
					}

					return false;
				} );
			}

			OnChange().Broadcast();

			return false;
		}

		/** These two functions are used to unwind a variadic list of arguments to Append */
		template <typename T0>
		void Append_internal( const T0& Arg )
		{
			Data.Add( Arg );
		}

		template <typename T0, typename... TS>
		void Append_internal( const T0& Arg0, const TS&... Args )
		{
			Append_internal( Arg0 );
			Append_internal( Args... );
		}

		FOnChange OnChangeEvent;
		FDelegateHandle TickHandle;
		ArrayType Data;
	};

	template <typename T>
	class TDataProviderUObject : public TDataProviderInternal<T>, public FGCObject
	{
		using Super = TDataProviderInternal<T>;
		TDataProviderUObject& operator=( const TDataProviderUObject& ) = delete;

	protected:
		using TDataProviderInternal<T>::Data;

		void AddReferencedObjects( FReferenceCollector& Collector ) override 
		{ 
			if ( Data.Num() > 0 )
			{
				Collector.AddReferencedObjects( Data );
			}
		}
	};

	template <typename T, bool bDerived>
	struct TDataProviderParent;

	template <typename T>
	struct TDataProviderParent<T, true>
	{
		using Type = TDataProviderUObject<T>;
	};

	template <typename T>
	struct TDataProviderParent<T, false>
	{
		using Type = TDataProviderInternal<T>;
	};

	template <typename T>
	using TParentClass = typename TDataProviderParent<T, TIsDerivedFrom<typename TRemovePointer<T>::Type, UObject>::IsDerived>::Type;
}

template <typename T>
class TDataProvider : public DPPrivate::TParentClass<T>
{
	using Super = DPPrivate::TParentClass<T>;
	TDataProvider& operator=( const TDataProvider& ) = delete;
};