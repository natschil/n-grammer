/* Experimental replacement for std::map optimized specifically for the n-gram.counter
 * Basically we implement a 256-ary trie for the first TOP_TRIE_LEVEL inputs followed by red black trees
 * for whatever is left.
 */

#define TOP_TRIE_LEVEL

class triemap
{
	public: 
		triemap(size_t numelements);
		mark_word(const word* new_word);
	private:
		void* memory;
}
