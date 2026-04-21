#include <bits/stdc++.h>
using namespace std;

// Persistent Treap-based ordered set supporting copy-on-write semantics via shared_ptr.
// Operations: insert (emplace), erase, find, range count, predecessor (--it), successor (++it).

struct Node {
    long long key;
    uint32_t prior;
    shared_ptr<Node> l, r;
    int sz;
    Node(long long k, uint32_t p, shared_ptr<Node> l=nullptr, shared_ptr<Node> r=nullptr)
        : key(k), prior(p), l(l), r(r), sz(1) {}
};

static inline int getsz(const shared_ptr<Node>& t){ return t? t->sz : 0; }
static inline shared_ptr<Node> upd(const shared_ptr<Node>& t){ if(!t) return t; auto n = make_shared<Node>(*t); n->sz = 1 + getsz(n->l) + getsz(n->r); return n; }

static shared_ptr<Node> rotate_right(const shared_ptr<Node>& t){ auto n = upd(t); auto L = upd(n->l); n->l = L? L->r : nullptr; if(n) n->sz = 1 + getsz(n->l) + getsz(n->r); if(L){ L->r = n; L->sz = 1 + getsz(L->l) + getsz(L->r); } return L? L : n; }
static shared_ptr<Node> rotate_left(const shared_ptr<Node>& t){ auto n = upd(t); auto R = upd(n->r); n->r = R? R->l : nullptr; if(n) n->sz = 1 + getsz(n->l) + getsz(n->r); if(R){ R->l = n; R->sz = 1 + getsz(R->l) + getsz(R->r); } return R? R : n; }

static shared_ptr<Node> insert_t(shared_ptr<Node> t, long long key, uint32_t pr){
    if(!t) return make_shared<Node>(key, pr);
    t = upd(t);
    if(key == t->key){ return t; }
    if(key < t->key){ t->l = insert_t(t->l, key, pr); if(t->l->prior > t->prior) t = rotate_right(t); }
    else{ t->r = insert_t(t->r, key, pr); if(t->r->prior > t->prior) t = rotate_left(t); }
    t->sz = 1 + getsz(t->l) + getsz(t->r);
    return t;
}

static shared_ptr<Node> erase_t(shared_ptr<Node> t, long long key){
    if(!t) return t;
    t = upd(t);
    if(key < t->key){ t->l = erase_t(t->l, key); }
    else if(key > t->key){ t->r = erase_t(t->r, key); }
    else{
        if(!t->l) return t->r;
        if(!t->r) return t->l;
        if(t->l->prior > t->r->prior){ t = rotate_right(t); t->r = erase_t(t->r, key); }
        else{ t = rotate_left(t); t->l = erase_t(t->l, key); }
    }
    t->sz = 1 + getsz(t->l) + getsz(t->r);
    return t;
}

static bool find_t(const shared_ptr<Node>& t, long long key){
    auto cur = t;
    while(cur){ if(key == cur->key) return true; cur = key < cur->key ? cur->l : cur->r; }
    return false;
}

static int count_range(const shared_ptr<Node>& t, long long L, long long R){
    if(!t) return 0;
    if(t->key < L) return count_range(t->r, L, R);
    if(t->key > R) return count_range(t->l, L, R);
    return 1 + count_range(t->l, L, R) + count_range(t->r, L, R);
}

static bool predecessor(const shared_ptr<Node>& t, long long key, long long &ans){
    auto cur = t; bool ok=false; long long best=0;
    while(cur){ if(cur->key < key){ ok=true; best=cur->key; cur = cur->r; } else cur = cur->l; }
    if(ok) ans=best; return ok;
}

static bool successor(const shared_ptr<Node>& t, long long key, long long &ans){
    auto cur = t; bool ok=false; long long best=0;
    while(cur){ if(cur->key > key){ ok=true; best=cur->key; cur = cur->l; } else cur = cur->r; }
    if(ok) ans=best; return ok;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector< shared_ptr<Node> > sets(1000005); // up to ~1e6 nodes in worst doc; index up to ~24 in sample
    // Iterator validity emulation: speedtest maintains a current iterator when last find/emplace success.
    bool valid=false; int it_a=-1; long long it_key=0;
    int op; int lst=0; int cnt=1;
    while(cin>>op){
        long long a,b,c;
        switch(op){
            case 0: { // emplace a b
                cin>>a>>b;
                bool existed = find_t(sets[a], b);
                if(!existed){
                    uint32_t pr = ((uint32_t)rand()<<1) ^ (uint32_t)rand();
                    sets[a] = insert_t(sets[a], b, pr);
                    it_a = (int)a; it_key=b; valid=true;
                }
                break;
            }
            case 1: { // erase a b
                cin>>a>>b;
                if(valid && it_a==(int)a && it_key==b) valid=false;
                if(find_t(sets[a], b)) sets[a]=erase_t(sets[a], b);
                break;
            }
            case 2: { // copy: s[++lst] = s[a]
                cin>>a;
                lst++;
                sets[lst] = sets[a];
                break;
            }
            case 3: { // find a b -> print true/false and set iterator
                cin>>a>>b;
                if(find_t(sets[a], b)){
                    cout<<"true\n";
                    it_a=(int)a; it_key=b; valid=true;
                }else{
                    cout<<"false\n";
                }
                cnt++;
                break;
            }
            case 4: { // range a b c
                cin>>a>>b>>c;
                int res = 0;
                if(b<=c) res = count_range(sets[a], b, c);
                cout<<res<<"\n";
                cnt++;
                break;
            }
            case 5: { // --it and print
                if(valid){
                    long long ans;
                    // predecessor of current key; also need to ensure iterator belongs to set a
                    if(predecessor(sets[it_a], it_key, ans)){
                        it_key = ans; cout<<it_key<<"\n";
                    }else{ valid=false; cout<<-1<<"\n"; }
                }else{
                    cout<<-1<<"\n";
                }
                cnt++;
                break;
            }
            case 6: { // ++it and print
                if(valid){
                    long long ans;
                    if(successor(sets[it_a], it_key, ans)){
                        it_key = ans; cout<<it_key<<"\n";
                    }else{ valid=false; cout<<-1<<"\n"; }
                }else{
                    cout<<-1<<"\n";
                }
                cnt++;
                break;
            }
            default: break;
        }
    }
    return 0;
}

