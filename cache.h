//cache.h
#ifndef CACHE_H
#define CACHE_H
#include <cstddef>
#include <list>
#include <vector>
#include <unordered_map>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#define  MUTEX  boost::lock_guard <boost::mutex> lock(this->mutex);

template< class Key, class Value > class Cache {
	public:
               /* Typedefs for ease of use */
		typedef std::list< std::pair< Key, Value> > list_t;         
		typedef typename list_t::iterator list_it;               
		typedef std::unordered_map< Key, list_it > map_t;                  
                typedef typename map_t::iterator map_it; 

	private:
		list_t cache;               // Cache list
		map_t index;               // Index map corresponds to Cache list
		unsigned long max_cache_size;  
		unsigned long cur_cache_size; 
		boost::mutex mutex;

	public:
 
               /* Constructor*/
		Cache( const unsigned long Size ) :
				max_cache_size(Size),
				cur_cache_size(0)
				{
                                }

		/* Destructor */
		~Cache() { clear(); }

		void clear( void ) {
			MUTEX;
			cache.clear();
			index.clear();
		};

		inline bool exists( const Key &key ) {
			MUTEX;
			return index.find( key ) != index.end();
		}


		inline void insert( const Key &key, const Value &value ) {
			MUTEX;
			auto m_iter = touch( key );
			if( m_iter != index.end() )
				evict( m_iter );

			cache.push_front( std::make_pair( key, value ) );
			auto l_iter = cache.begin();

			index.insert( std::make_pair( key, l_iter ) );
			cur_cache_size++;

			while( cur_cache_size > max_cache_size) {
				// Least Recently used (LRU) eviction.
				l_iter = cache.end();
				--l_iter;
				evict( l_iter->first );
			}
		}

		inline Value get_value( const Key &key ) {
			MUTEX;
			auto m_iter = index.find(key);
			if( m_iter == index.end() )
				return Value ();
			Value tmp = m_iter->second->second;
			touch(key);
			return tmp;
		}

	private:
		inline  map_it touch( const Key &key ) {
			auto m_iter = index.find( key );
			if( m_iter == index.end() ) return m_iter;
			cache.splice( cache.begin(), cache, m_iter->second );
			return m_iter;
		}

		inline void evict( const  map_it  &m_iter ) {
                         cur_cache_size --;
			cache.erase( m_iter->second);
			index.erase( m_iter );
		}

		inline void evict( const Key &key ) {
			auto m_iter = index.find( key );
			evict( m_iter );
		}
};

#endif /* CACHE_H*/
