#include <bits/stdc++.h>
using namespace std;
// Faster persistent treap with raw pointers and split/merge. Minimizes overhead vs shared_ptr.
struct Node {
    long long key;
    uint32_t prior;
    Node *l, *r;
    int sz;
};

// Arena allocator to reduce per-node overhead.
static vector<char*> g_blocks;
static char* g_cur = nullptr;
static size_t g_left = 0;
static const size_t BLOCK_BYTES = 1u<<22; // 4 MiB per block

static inline void ensure_block(){
    if(g_left < sizeof(Node)){
        char* buf = new char[BLOCK_BYTES];
        g_blocks.push_back(buf);
        g_cur = buf;
        g_left = BLOCK_BYTES;
    }
}

static inline int getsz(Node* t){ return t? t->sz : 0; }
static inline Node* make_node(long long k, uint32_t p, Node* l=nullptr, Node* r=nullptr){
    ensure_block();
    Node* n = reinterpret_cast<Node*>(g_cur);
    g_cur += sizeof(Node); g_left -= sizeof(Node);
    n->key = k; n->prior = p; n->l = l; n->r = r; n->sz = 1 + getsz(l) + getsz(r);
    return n;
}
static inline Node* clone(Node* t){ if(!t) return nullptr; ensure_block(); Node* n = reinterpret_cast<Node*>(g_cur); g_cur += sizeof(Node); g_left -= sizeof(Node); *n = *t; return n; }
static inline void pull(Node* t){ if(t) t->sz = 1 + getsz(t->l) + getsz(t->r); }

// xorshift RNG
static inline uint32_t rng(){
    static uint64_t s = 0x9E3779B97F4A7C15ULL;
    s ^= s << 7; s ^= s >> 9; s ^= s << 8;
    return (uint32_t)(s + (s>>32));
}

// Split into (< key, >= key) when ge_right=true; else (< key, > key) if inclusive=false using <= variant below
static pair<Node*,Node*> split_lt(Node* t, long long key){
    if(!t) return {nullptr,nullptr};
    if(t->key >= key){
        Node* n = clone(t);
        auto pr = split_lt(t->l, key);
        n->l = pr.second; pull(n);
        return {pr.first, n};
    }else{
        Node* n = clone(t);
        auto pr = split_lt(t->r, key);
        n->r = pr.first; pull(n);
        return {n, pr.second};
    }
}

// Split into (<= key, > key)
static pair<Node*,Node*> split_le(Node* t, long long key){
    if(!t) return {nullptr,nullptr};
    if(t->key > key){
        Node* n = clone(t);
        auto pr = split_le(t->l, key);
        n->l = pr.second; pull(n);
        return {pr.first, n};
    }else{
        Node* n = clone(t);
        auto pr = split_le(t->r, key);
        n->r = pr.first; pull(n);
        return {n, pr.second};
    }
}

static Node* merge(Node* a, Node* b){
    if(!a) return b; if(!b) return a;
    if(a->prior > b->prior){
        Node* n = clone(a);
        n->r = merge(n->r, b); pull(n); return n;
    }else{
        Node* n = clone(b);
        n->l = merge(a, n->l); pull(n); return n;
    }
}

static bool find_t(Node* t, long long key){
    while(t){ if(key==t->key) return true; t = (key < t->key) ? t->l : t->r; }
    return false;
}

static int count_leq(Node* t, long long x){
    int cnt=0; while(t){ if(t->key <= x){ cnt += 1 + getsz(t->l); t=t->r; } else t=t->l; } return cnt;
}

static bool predecessor(Node* t, long long key, long long &ans){
    bool ok=false; long long best=0; while(t){ if(t->key < key){ ok=true; best=t->key; t=t->r; } else t=t->l; } if(ok) ans=best; return ok;
}
static bool successor(Node* t, long long key, long long &ans){
    bool ok=false; long long best=0; while(t){ if(t->key > key){ ok=true; best=t->key; t=t->l; } else t=t->r; } if(ok) ans=best; return ok;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Node*> sets(1, nullptr);
    bool valid=false; int it_a=-1; long long it_key=0;
    int op; int lst=0; int cnt=1;
    while((cin>>op)){
        long long a,b,c;
        switch(op){
            case 0: { // emplace a b
                cin>>a>>b; int ia=(int)a; if(ia >= (int)sets.size()) sets.resize(ia+1, nullptr);
                if(!find_t(sets[ia], b)){
                    auto pr = split_lt(sets[ia], b);
                    Node* mid = make_node(b, rng());
                    sets[ia] = merge(merge(pr.first, mid), pr.second);
                    it_a = ia; it_key = b; valid = true;
                }
                break;
            }
            case 1: { // erase a b
                cin>>a>>b; int ia=(int)a; if(ia >= (int)sets.size()) sets.resize(ia+1, nullptr);
                if(valid && it_a==ia && it_key==b) valid=false;
                if(find_t(sets[ia], b)){
                    auto pr1 = split_lt(sets[ia], b);
                    auto pr2 = split_le(pr1.second, b); // pr2.first are ==b
                    // drop pr2.first
                    sets[ia] = merge(pr1.first, pr2.second);
                }
                break;
            }
            case 2: { // copy: s[++lst] = s[a]
                cin>>a; int ia=(int)a;
                lst++;
                if(lst >= (int)sets.size()) sets.resize(lst+1, nullptr);
                if(ia >= (int)sets.size()) sets.resize(ia+1, nullptr);
                sets[lst] = sets[ia];
                break;
            }
            case 3: { // find a b
                cin>>a>>b; int ia=(int)a; if(ia >= (int)sets.size()) sets.resize(ia+1, nullptr);
                if(find_t(sets[ia], b)){
                    cout << "true\n"; it_a=ia; it_key=b; valid=true;
                }else{
                    cout << "false\n";
                }
                cnt++; break;
            }
            case 4: { // range a b c
                cin>>a>>b>>c; int ia=(int)a; if(ia >= (int)sets.size()) sets.resize(ia+1, nullptr);
                int res=0; if(b<=c){ if(b==LLONG_MIN) res = count_leq(sets[ia], c); else res = count_leq(sets[ia], c) - count_leq(sets[ia], b-1); }
                cout << res << "\n"; cnt++; break;
            }
            case 5: { // --it
                if(valid){ long long ans; if(predecessor(sets[it_a], it_key, ans)){ it_key=ans; cout<<it_key<<"\n"; } else { valid=false; cout<<-1<<"\n"; } }
                else { cout<<-1<<"\n"; }
                cnt++; break;
            }
            case 6: { // ++it
                if(valid){ long long ans; if(successor(sets[it_a], it_key, ans)){ it_key=ans; cout<<it_key<<"\n"; } else { valid=false; cout<<-1<<"\n"; } }
                else { cout<<-1<<"\n"; }
                cnt++; break;
            }
            default: break;
        }
    }
    // Free all arena blocks
    for(char* b : g_blocks) delete [] b;
    return 0;
}
