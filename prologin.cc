// Note : Ceci n'est pas la version finale utilisé, la seule différence étant la valeurs des coefficients.

#include "prologin.hh"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <cmath>

/*
 * README:
 * Strategie de ce bot :
 * On coupe en deux la strategie de don d'echantillon et de placement/transmutation/catalyse.
 * 
 * Preliminaire :
 *   Un concept tres important pour cette IA est la notion de _score de plateau/etabli_
 *   Le principe est de donner un score a un etabli sans prendre en compte le reste du contexte (echantillon recu, echantillon donne, etc..)
 *   Un grand temps a ete passe a paufiner cette fonction et a ajouter pleins de petites informations.
 *   De telles informations inclues : 
 *   - Le nombre d'or/catalyse rapportee si on vidait le plateau
 *   - Le nombre de "regions", cad de blocs adjaceant de meme type (on enleve une region lorsqu'on transmutte)
 *   - Le nombre de trou, cad de regions vides isolees
 *   - De combien les regions sont etires sur le plateau (comme des serpents)
 *   - Le nombre de cases vide a cote de chaque region (isolement)
 *   
 * Strategie de placement : 
 *   Le principe (et comme pour toutes les autres strategie) est de raisonner en fonction du score de plateau.
 *   Ainsi pour le placement, on itere toutes les possibilitees de placement d'echantillon et on regarde celle qui donne le meilleur score
 *   A noter qu'on itere aussi les placements qui incluent une transmutation au prealable. (avec un malus pour ceux la)
 * 
 * Strategie de catalyse (defense) :
 *   Idem que pour le placement, on itere toutes les manieres de catalyser, et on prend la meilleur.
 * 
 * Strategie de catalyse (attaque) :
 *   Comme pour la strategie de defense, simplement on prend celle qui fait perdre le plus de points a l'adversaire
 * 
 * Strategie de transmutation : 
 *   Il n'y a en fait pas a proprement parler de strategie de transmutation, cellent ci sont automatiquement effectuees dans 
 *   la strategie de placement (a partir d'une certaine taille, detruire une region n'est plus un malus)
 *   
 *   En revanche, il existe un autre cas ou une transmutation va s'effectuer (decrite par la fonction decoincer)
 *   Le principe etant que si on possede une grande (>10) region dont l'adversaire ne nous as pas donne le type dans ses echantillons depuis plus de 5 tours, 
 *   alors on suppose qu'on ne pourra plus la faire grandir, et ainsi on la transmute.
 *   Cette technique date du debut du concours, mais l'enlever faisait perdre l'IA dans certains cas.
 * 
 * Toutes ces strategies sont decrites un peu plus en detail dans la documentation des fonctions.
 * 
 * Strategie de don : 
 *   La strategie de don se rapproche aussi beaucoup du reste.
 *   On itere parmis touts les echantillons donnables (10), puis on se mets a la place de l'adversaire
 *   en essayant de placer cette echantillon de la meilleur maniere possible.
 *   On choisit ainsi l'echantillon qui a mis l'adversaire dans le plus de difficulte.
 * 
 * Strategie de defense par la fuite :
 *   Tout d'abord, il sagit d'identifier si l'ennemi est offensif ou non.
 *   Un enemi est considere offensif si il catalyse au moins une fois le plateau allie, ou qu'il construit 
 *   une tres grande region de mercure/soufre.
 *   
 *   Une fois que l'ennemi est identifie comme dangereux, alors on effectue la strategie de defense:
 *   Le principe est que si l'on detecte qu'au tour suivant il pourrait nous detruire notre magnifique region metalique qui a un fort potentiel, 
 *   alors on transmute cette zone a risque, afin de recuperer tout l'or possible.
 *   Bien sur, cela est tres difficile a equilibrer, car si l'on est trop sensible alors on ne profitera pas de la quadracite de l'or de regions solide.
 *   En revanche, si l'on est pas assez sensible, alors on risque d'etre trop ambitieux tout cela pour se faire marcher dessus par les catalyseurs ennemis.
 *
 * Si j'avais eu plus de temps :
 *  Si j'avais eu plus de temps, j'aurai ajoute une feature auquel j'avais pense au debut du concours.
 *  Pour l'instant, l'ordre est toujours le meme :
 *  
 *   - si dangereux:
 *       se proteger
 *   - se decoincer
 *   - placer
 *   - utiliser cataliseurs
 *   - transmuter le plateau (si c'est le dernier tour)
 *   - determiner quoi donner
 *   - catalyser ce qu'il reste
 *   
 *   Ainsi par exemple, une chose que j'ai vu chez d'autres bots mais que je n'aurais pas pu faire a cause de la structure est :
 *   - transmuter
 *   - catalyser
 *   - placer
 *   J'aurais donc essaye d'explorer d'autres ordres (il n'y en as pas tant que ca qui sont intelligents), et prendre l'ordre le plus adequat
 * 
 *   Un autre feature aurait ete d'unifier l'attaque et la defense de la catalyse, car pour l'instant l'ordre est relativement arbitraire.
 *   Il se trouve que j'ai essaye d'implementer cette feature mais elle ne donnait pas de bons resultats a cause de la non-linearite de l'algorithme de score de plateau 
 *   (cela empeche de comparer des gains)
 * 
 * PS:
 *   Une grande decouverte cette annee qui a grandement aide a la determination des coefficients et algorithme 
 *   est le principe de faire jouer le bot contre lui meme (a l'aide d'un script, sur environ 100-200 parties a chaque fois). 
 *   Ainsi, a chaque fois qu'une fonctionnalite etait implemente, je verifiais qu'il n'y avais eu aucune regression en faisant 
 *   se battre la nouvelle IA contre l'IA precedente.
 *   Cela a notamment permis d'eliminer le mix des deux catalyse (catalyse defensive et offensive) qui auraient
 *   mene ce bot a sa perte.
 * 
 *   90% des coeffs ont ainsi naturellement ete determine en utilisant cette technique.
 */

using namespace std;
// types pratique

using etabli = int[36];
// utilise pour calculer les regions de maniere efficace
using etabli_visit = bool[36];
using region = vector<int>;
typedef struct {int gold; int cat;} gold_cat;

// variables globales
// Liste de toutes les positions possibles d'echantillons sur le plateau
vector<position_echantillon> all_pos;
// Historique des echantillons donnes par l'adversaire
vector<echantillon> ech_hist;
// Indique si l'adversaire est dangereux ou non
bool dangereux = false;

// conversion pour les formats de l'API
position to_position(int pos) {
    return (position){pos/6, pos%6};
}

int from_position(position pos) {
    return pos.ligne*6 + pos.colonne;
}

// Affichage de differents elements pour le debug
void print_etabli(etabli e) {
    for(int l = 0 ; l < 6 ; l++) {
        for(int c = 0 ; c < 6 ; c++) {
            cout << e[l*6+c] << " ";
        }
        cout << endl;
    }
}

void print_region(region g) {
    cout << "[";
    if(g.size() > 0) {
        cout << g[0];
        for(unsigned int i = 1 ; i < g.size() ; i++) {
            cout << ", " << g[i];
        }
    }
    cout << "]";
}

void print_vector(vector<int> g) {
    cout << "[";
    if(g.size() > 0) {
        cout << g[0];
        for(unsigned int i = 1 ; i < g.size() ; i++) {
            cout << ", " << g[i];
        }
    }
    cout << "]";
}

void print_regions(vector<region> g) {
    cout << "[";
    if(g.size() > 0) {
        print_region(g[0]);
        for(unsigned int i = 1 ; i < g.size() ; i++) {
            cout << ", ";
            print_region(g[i]);
        }
    }
    cout << "]";
}

// fonctions d'initialisation

void init_etabli(etabli& e, int apprentiId) {
    for(int i = 0 ; i < 36 ; i++) {
        e[i] = (int)api_type_case(to_position(i), apprentiId);
    }
}

void init_etabli_visit(etabli_visit& e) {
    for(int i = 0 ; i < 36 ; i++)
        e[i] = false;
}

void partie_init()
{
    ech_hist = vector<echantillon>();
    all_pos = vector<position_echantillon>();
    for(int c = 0 ; c < 6 ; c++)
        for(int l = 0 ; l < 5 ; l++) {
            all_pos.push_back({{l, c}, {l+1, c}});
            all_pos.push_back({{l+1, c}, {l, c}});
        }
    for(int c = 0 ; c < 5 ; c++)
        for(int l = 0 ; l < 6 ; l++) {
            all_pos.push_back({{l, c}, {l, c+1}});
            all_pos.push_back({{l, c+1}, {l, c}});
        }
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::srand((int)(ms*10000));
}

// Fonctions pratiques

// Donne la liste des voisins d'un emplacement (aurait pu etre precalcule)
vector<int> neighbours(etabli e, int p) {
    vector<int> pos = vector<int>();
    if(p%6 > 0) {
        pos.push_back(p-1);
    }
    if(p/6 > 0) {
        pos.push_back(p-6);
    }
    if(p%6 < 5) {
        pos.push_back(p+1);
    }
    if(p/6 < 5) {
        pos.push_back(p+6);
    }
    return pos;
}

// Recupere une region (ensemble de couleurs adjacentes) d'un etabli a une position donne.
// etabli_visit est utilise pour etre plus efficace lors de l'utilisation de la fonction regions
// une region est un vecteur de position
region get_region(etabli e, etabli_visit& vis, int pos) {
    region g = region();
    vector<int> cur = vector<int>();
    vector<int> next = vector<int>();
    int type = e[pos];
    
    cur.push_back(pos);
    vis[pos] = true;
    
    // Flood fill classique..
    while(cur.size() > 0) {
        next.clear();
        for (int elem : cur) {  
            g.push_back(elem);
            for(int nei : neighbours(e, elem)) {
                if(!vis[nei] && e[nei] == type) {
                    next.push_back(nei);
                    vis[nei] = true;
                }
            }
        }
        cur.swap(next);
    }
    
    return g;
}

// Renvoie la liste des regions d'un etabli
vector<region> regions(etabli e) {
    etabli_visit visit;
    init_etabli_visit(visit);
    vector<region> regions = vector<region>();
    for(int i = 0 ; i < 36 ; i++) {
        if(!visit[i] && e[i] > 0 ) {
            regions.push_back(get_region(e, visit, i));
        }
    }
    return regions;
}

// Renvoie le score d'une region, cad le nombre d'or et de catalyseur renvoye par celui ci
gold_cat score_region(etabli e, region g) {
    int len = g.size();
    if(len == 0) {
        return {-3, 0};
    }
    if(e[g[0]] <= 3) {
        return {api_quantite_transmutation_or(len), 0};
    }
    return {api_quantite_transmutation_catalyseur_or(len),
            api_quantite_transmutation_catalyseur(len)};
}

// Trouve la region rapportant le plus d'or
region find_best_region_gold(vector<region> regions, etabli e) {
    int max = 0;
    region best;
    
    for(region g : regions) {
        gold_cat score = score_region(e, g);
        if(score.gold >= max) {
            max = score.gold;
            best = g;
        }
    }
    
    return best;
}


// Calcule l'isolement d'une region, cad le nombre de case vide adjacentes a une region
// Ce nombre est dnoc faible pour une region tres isole, et grand pour une region peu isole
int isolement_region(region r, etabli etab) {
    int sum = 0;
    for(int i = 0 ; i < 36 ; i++) {
        if(etab[i] == 0) {
            for(int p : neighbours(etab, i)) {
                for(int k : r) {
                    if(k == p) {
                        sum += 1;
                        break;
                    }
                }
            }
        }
    }
    return sum;
}

// Coefficients divers 
const float COEFF_CATA = 1.1;
const float COEFF_GOLD = 1.8;
const float COEFF_REGI = 6;
const float COEFF_ISOLEMENT = 2;
const float COEFF_TROU = 1.3;
const float COEFF_SERPENT = 0.3;


// Renvoie le score d'un etabli comme la somme de divers sous-score
// Les sous scores incluent : 
// - Le nombre d'or/catalyseur rapporte par les regions
// - L'isolement de chaque region
// - La "serpencite" de chaque region, cad a quel point celle ci est etire.
// - Le nombre de regions
// - Le nombre de "trous", cad les cases vide ayant peu de cases vide adjacentes
float score_etabli(etabli etab) {
    float score = 0;
    vector<region> regs = regions(etab);
    for(region r : regs) {
        gold_cat score_r = score_region(etab, r);
        int type = etab[r[0]];
        if(score_r.gold == -3)
            score_r.gold = -0.02;
        int isolement = isolement_region(r, etab)/(2.5+r.size());
        
        float serpent = 0;
        for(int k : r) {
            int sum = 0;
            for(int ne : neighbours(etab, k)) {
                if(etab[ne] == type) {
                    sum += 1;
                }
            }
            serpent += sum;
        }
        serpent /= r.size();
        score += score_r.gold*COEFF_GOLD + score_r.cat*COEFF_CATA + isolement*COEFF_ISOLEMENT + serpent*COEFF_SERPENT; // SCORE TODO
    }
    score -= regs.size()*COEFF_REGI;
    float trous = 0;
    for(int i = 0 ; i < 36 ; i++) {
        if(etab[i] == 0) {
            vector<int> n = neighbours(etab, i);
            int sum = 0;
            for(int k : n) {
                if(etab[k] == 0) {
                    sum++;
                }
            }
            if(sum == 0) {
                trous += 1;
            }
            if(sum == 1) {
                trous += .3;
            }
            if(sum == 2) {
                trous += .001;
            }
        }
    }
    score -= trous*COEFF_TROU;
    return score;
}

// Debug les stats du score d'un etabli
void score_stats(etabli etab) {
    float score = 0;
    vector<region> regs = regions(etab);
    cout << "Il y a " << regs.size() << " regions" << endl;
    for(region r : regs) {
        gold_cat score_r = score_region(etab, r);
        int type = etab[r[0]];
        if(score_r.gold == -3)
            score_r.gold = 0;
        int isolement = isolement_region(r, etab)/(3+r.size());
        
        float serpent = 0;
        for(int k : r) {
            int sum = 0;
            for(int ne : neighbours(etab, k)) {
                if(etab[ne] == type) {
                    sum += 1;
                }
            }
            serpent += sum;
        }
        serpent /= r.size();
        
        cout << "  " << score_r.gold*COEFF_GOLD << "g " << score_r.cat*COEFF_CATA << "c " << isolement*COEFF_ISOLEMENT << "i " << endl;
        score += score_r.gold*COEFF_GOLD + score_r.cat*COEFF_CATA + isolement*COEFF_ISOLEMENT; // SCORE TODO
    }
    cout << "total: " << score << endl;
    score -= regs.size()*COEFF_REGI;
    cout << "region size: " << regs.size()*COEFF_REGI << endl;
    float trous = 0;
    for(int i = 0 ; i < 36 ; i++) {
        if(etab[i] == 0) {
            vector<int> n = neighbours(etab, i);
            int sum = 0;
            for(int k : n) {
                if(etab[k] == 0) {
                    sum++;
                }
            }
            if(sum == 0) {
                trous += 1;
            }
            if(sum == 1) {
                trous += .3;
            }
            if(sum == 2) {
                trous += .001;
            }
        }
    }
    score -= trous*COEFF_TROU;
    cout << "trous: " << trous*COEFF_TROU << endl;
    cout << "final: " << score << endl;
}



// Applique la pose d'un echantillon sur un etabil
void appliquer_etabli(echantillon ech, position_echantillon pe, etabli& e) {
    e[from_position(pe.pos1)] = ech.element1;
    e[from_position(pe.pos2)] = ech.element2;
}

// Applique la transmutation d'une position sur un etabli
void appliquer_transmuter(etabli& e, int pos) {
    vector<int> cur = vector<int>();
    vector<int> next = vector<int>();
    vector<int> todel = vector<int>();
    etabli_visit vis;
    init_etabli_visit(vis);
    int type = e[pos];
    
    cur.push_back(pos);
    vis[pos] = true;
    // Flood fill classique..
    while(cur.size() > 0) {
        next.clear();
        for (int elem : cur) {  
            todel.push_back(elem);
            for(int nei : neighbours(e, elem)) {
                if(!vis[nei] && e[nei] == type) {
                    next.push_back(nei);
                    vis[nei] = true;
                }
            }
        }
        cur.swap(next);
    }
    for(int i : todel) {
        e[i] = 0;
    }
}
// Indique si une pose d'echantillon sur un etabli est valide ou non
bool est_valide(etabli e, position_echantillon pe, echantillon ech) {
    int p1 = from_position(pe.pos1);
    int p2 = from_position(pe.pos2);
    
    if(e[p1] != 0 or e[p2] != 0)
        return false;
    bool libre = true;
    for(int i = 0 ; i < 36 ; i++) {
        if(e[i] == ech.element1 or e[i] == ech.element2)
        {
            libre = false;
            break;
        }
    }
    if(libre)
        return true;
    for(int k : neighbours(e, p1))
        if(k != p2 && e[k] == ech.element1)
            return true;
    for(int k : neighbours(e, p2))
        if(k != p1 && e[k] == ech.element2)
            return true;
    return false;
}

void transmuter_fin();

// Place l'echantillon sur le plateau
// L'algorithme de placement est relativement simple
// Pour chaque position valide d'echantillon (en incluant le fait de transmuter puis placer):
//    On simule l'etabli dans cette forme
//    On regarde le score de l'etabli (avec un petit malus si on a transmute, sauf si c'est une grosse transmutation)
// On renvoie le maximum de tuot ces scores
void placer() {
    etabli etab2;
    etabli_visit vis;
    echantillon ech = api_echantillon_tour();
    
    vector<position_echantillon> best_echs;
    float max = -1000000;
    int somme_egaux = 0;
    for(position_echantillon pe : all_pos) {
        init_etabli(etab2, api_moi());
        init_etabli_visit(vis);
        /*cout << "--" << endl;
        print_etabli(etab2);
        cout << from_position(pe.pos1) << " " << etab2[from_position(pe.pos1)] << " ";
        cout << from_position(pe.pos2) << " " << etab2[from_position(pe.pos2)] << " ";
        */
        float malus_replace = 0;
        if(etab2[from_position(pe.pos1)] > 0) {
            int size = get_region(etab2, vis, from_position(pe.pos1)).size();
            //if(size > 8)
            //    size = 8;
            size -= 5;
            size *= abs(size);
            malus_replace -= size - 5;
            appliquer_transmuter(etab2, from_position(pe.pos1));
        }
        if(etab2[from_position(pe.pos2)] > 0) {
            int size = get_region(etab2, vis, from_position(pe.pos2)).size();
            /*if(size > 8)
                size = 8;*/
            size -= 5;
            size *= abs(size);
            malus_replace -= size - 5;
            appliquer_transmuter(etab2, from_position(pe.pos2));
        }
        if(!est_valide(etab2, pe, ech))
            continue;
        appliquer_etabli(ech, pe, etab2);
        float score_ech = score_etabli(etab2) - malus_replace;
        if(score_ech == max) {
            somme_egaux += 1;
            best_echs.push_back(pe);
        }
        if(score_ech > max) {
            max = score_ech;
            best_echs.clear();
            best_echs.push_back(pe);
            somme_egaux = 1;
        }
    }
    // Pas de position trouvee, on nettoie le plateau car il sera detruit au prochain tour.
    if(max == -1000000) {
        transmuter_fin();
        return;
    }
    
    position_echantillon bestEch = best_echs[std::rand()%somme_egaux];
    /*if(somme_egaux > 1) {
        cout << somme_egaux << " egaux avec comme score " << max << " et best_pos: " << endl;
        for(position_echantillon kiwi : best_echs) {
           cout << from_position(kiwi.pos1)/6 << " " << from_position(kiwi.pos1)%6 << " " << from_position(kiwi.pos2)/6 << " " << from_position(kiwi.pos2)%6 << endl;
        }
        api_afficher_etablis();
        cout << endl;
    }*/
    if(!api_est_vide(bestEch.pos1, api_moi())) {
        api_transmuter(bestEch.pos1);
    }
    if(!api_est_vide(bestEch.pos2, api_moi())) {
        api_transmuter(bestEch.pos2);
    }
    
    api_placer_echantillon(bestEch.pos1, bestEch.pos2);
}

void catalyser_pour_transmuter(bool only_metaux);

// Fonction de nettoyage de plateau (pour le dernier tour notamment)
// Le principe est simple :
// 1- On enleve toutes les regions a mercures (pour les catalystes)
// 2- On catalyse en optimisants les metaux (pour faire de l'or)
// 3- On enleve le reste
void transmuter_fin() {
    etabli e;
    while(true) {
        init_etabli(e, api_moi());
        region best;
        float max = 0;
        for(region r : regions(e)) {
            if(api_propriete_case_type((case_type)e[r[0]]) == TRANSMUTABLE_CATALYSEUR) {
                float score = r.size();
                if(score > max)
                {
                    max = score;
                    best = r;
                }
            }
        }
        if(max <= 1)
        {
            break;
        }
        api_transmuter(to_position(best[0]));
    }
    for(int i = 0 ; i < api_nombre_catalyseurs() ; i++) {
        catalyser_pour_transmuter(true);
    }
    init_etabli(e, api_moi());
    for(region r : regions(e)) {
        if(r.size() >= 2) {
            api_transmuter(to_position(r[0]));
        }
    }
}

// Fonction dont l'objectif est d'utiliser un catalyste pour aider a une meilleur transmutation
// Elle cherche parmis toutes les possibilitees de catalysations celle qui rapporte le plus de point
// C'est un comportement tres similaire a celui de "placer"
void catalyser_pour_transmuter(bool only_metaux) {
    int catalNb = api_nombre_catalyseurs();
    if(catalNb <= 0)
        return;
    cout << "J'ai " << catalNb << "catalyseurs" << endl;
    etabli etab2;
    init_etabli(etab2, api_moi());
    
    
    float max_score = score_etabli(etab2);
    int cat_pos = -1;
    int cat_typ = -1;
    
    for(int i = 0 ; i < 36 ; i++) {
        if(etab2[i] == 0)
            continue;
        for(int cat_id = 1  ; cat_id < 6 - ((only_metaux) ? 2 : 0 ); cat_id++) {
            if(cat_id == etab2[i])
                continue;
            int tmp = etab2[i];
            etab2[i] = cat_id;
            float score_ech = score_etabli(etab2);
            if(score_ech > max_score) {
                max_score = score_ech;
                cat_pos = i;
                cat_typ = cat_id;
            }
            etab2[i] = tmp;
        }
    }
    if(cat_pos != -1) {
        cout << "catalyse en " << cat_pos << " avec " << cat_typ << " score: " << max_score << endl;
        api_catalyser(to_position(cat_pos), api_moi(), (case_type)cat_typ);
    }
}

// Se comporte comme catalyser_pour_transmuter, mais essaie de reduire le score de l'ennemi le plus possible
void catalyser_pour_attaquer() {
    int catalNb = api_nombre_catalyseurs();
    if(catalNb <= 0)
        return;
    cout << "J'ai " << catalNb << " catalyseurs (mode attaque) " << endl;
    etabli etab2;
    init_etabli(etab2, api_adversaire());
    
    
    float min_score = score_etabli(etab2);
    int cat_pos = -1;
    int cat_typ = -1;
    
    for(int i = 0 ; i < 36 ; i++) {
        if(etab2[i] == 0)
            continue;
        for(int cat_id = 1  ; cat_id < 6 ; cat_id++) {
            if(cat_id == etab2[i])
                continue;
            int tmp = etab2[i];
            etab2[i] = cat_id;
            float score_ech = score_etabli(etab2);
            if(score_ech < min_score) {
                min_score = score_ech;
                cat_pos = i;
                cat_typ = cat_id;
            }
            etab2[i] = tmp;
        }
    }
    if(cat_pos != -1) {
        cout << "catalyse en " << cat_pos << " avec " << cat_typ << " (attaque) score: " << min_score << endl;
        api_catalyser(to_position(cat_pos), api_adversaire(), (case_type)cat_typ);
    }
}

// Fonction qui regarde si il y a un danger proche (cad que l'ennemi a beaucoup de mercure)
// Si c'est le cas, il essaie de transmuter les gros blocs, avec une fonction de risque
// qui regarde si cela vaut le coup de transmuter une region (la "sauvant" d'une potentielle attaque ennemie)
// L'algorithme prend simplement la plus grosse region, et si elle respecte certains criteres en fonction
// des degats (nombre de catalystes potentielle qu'a l'adversaire), alors elle la transmute.
void danger_transmut() {
    etabli e_enemi, e;
    init_etabli(e_enemi, api_adversaire());
    init_etabli(e, api_moi());
    
    float degats = 0;
    for(region r : regions(e_enemi)) {
        if(e_enemi[r[0]] >= SOUFRE) {
            degats = fmax(degats, (r.size()-1)/2);
            if(r.size()%2 == 1)
                degats += 0.05;
        }
    }
    if(degats == 0)
        return;
    vector<region> regs = regions(e);
    if(regs.size() == 0)
        return;
    region biggest;
    float max_s = 0;
    for(region r : regs) {
        gold_cat score_r = score_region(e, r);
        float score = score_r.gold*COEFF_GOLD + score_r.cat*COEFF_CATA;
        if(score > max_s) {
            biggest = r;
            max_s = score;
        }
    }
    //gold_cat score_r = score_region(e, biggest);
    cout << "Analyse de danger --" << endl;
    cout << "  region la plus grosse : " << biggest.size() << " (ok for >= " << fmax(6, 8 - (degats+1)/2.0) << ") ";
    print_region(biggest);
    cout << endl;
    if(biggest.size() >= fmax(6, 8 - (degats+1)/2.0)) {
        float score = biggest.size()*2.5;
        float diff = score - score/(0.6+0.8*degats);
        float test_diff = 7.1;
        cout << "  Danger transmut avancee: "<<endl;
        cout << "    score: " << score << endl;
        cout << "    size:  " << biggest.size() << endl;
        cout << "    diff:  " << diff << endl;
        cout << "    test:  " << test_diff << endl;
        cout << "    degat: " << degats << endl;
        if(diff >= test_diff) {
            cout << "  Transmutation!" << endl;
            api_transmuter(to_position(biggest[0]));
        }
    }
}

// Fonction de fallback si la fonction "donner2" n'a pas aboutie
void donner() {
    etabli e_enemi;
    init_etabli(e_enemi, api_adversaire());
    int count[6] = {0, 0, 0, 0, 0, 0};
    for(int i = 0 ; i < 36 ; i++) {
        count[e_enemi[i]]++;
    }
    echantillon don;
    echantillon e = api_echantillon_tour();
    
    if(count[e.element1] < count[e.element2]) {
        don.element1 = e.element1;
    } else {
        don.element1 = e.element2;
    }
    int min = 100;
    int minElem = 0;
    for(int i = 1 ; i < 6 ; i++) {
        if(count[i] < min && i != don.element1) {
            min = count[i];
            minElem = i;
        }
    }
    don.element2 = (case_type)minElem;
    donner_echantillon(don);
}

// Fonction qui donne un echantillon a l'adversaire
// Elle fonctionne presque comme la fonction placer
// Le principe est de regarder pour chaque echantillon comment l'adversaire le placerait (en utilisant la meme methode que pour la fonction "placer")
// Et on prend ensuite l'echantillon qui a le score le moins bon (on suppose ainsi que l'adversaire est aussi bon que nous)
void donner2() {
    echantillon e = api_echantillon_tour();
    echantillon don;
    etabli e_enemi;
    etabli_visit vis;
    init_etabli(e_enemi, api_adversaire());
    float min = 1000000;
    echantillon best_don;
    bool notok = true;
    
    for(int a = 0; a < 2 ; a++) {
        if(a == 1)
            don.element1 = e.element1;
        if(a == 0)
            don.element1 = e.element2;
        for(int k = 1 ; k < 6 ; k++) {
            don.element2 = (case_type)k;
            float max = -100000;
            position_echantillon best_pos = {-1, -1};
            
            for(position_echantillon pe : all_pos) {
                init_etabli(e_enemi, api_adversaire());
                init_etabli_visit(vis);
                /*cout << "--" << endl;
                print_etabli(etab2);
                cout << from_position(pe.pos1) << " " << etab2[from_position(pe.pos1)] << " ";
                cout << from_position(pe.pos2) << " " << etab2[from_position(pe.pos2)] << " ";
                */
                float malus_replace = 0;
                if(e_enemi[from_position(pe.pos1)] > 0) {
                    int size = get_region(e_enemi, vis, from_position(pe.pos1)).size();
                    /*if(size > 15)
                        size = 15;*/
                    size -= 5;
                    size *= abs(size);
                    malus_replace -= size - 5;
                    appliquer_transmuter(e_enemi, from_position(pe.pos1));
                }
                if(e_enemi[from_position(pe.pos2)] > 0) {
                    int size = get_region(e_enemi, vis, from_position(pe.pos2)).size();
                    /*if(size > 15)
                        size = 15;*/
                    size -= 5;
                    size *= abs(size);
                    malus_replace -= size - 5; 
                    appliquer_transmuter(e_enemi, from_position(pe.pos2));
                }
                if(!est_valide(e_enemi, pe, don))
                    continue;
                appliquer_etabli(don, pe, e_enemi);
                float score = score_etabli(e_enemi) - malus_replace;
                if(score > max) {
                    max = score;
                    best_pos = pe;
                    notok = false;
                }
            }
            if(don.element1 >= SOUFRE)
                max -= 0.01;
            if(don.element2 >= SOUFRE)
                max -= 0.01;
            if(max < min) {
                min = max;
                best_don = don;
            }
        }
    }
    if(notok) {
        donner();
    } else {
        donner_echantillon(best_don);
    }
}

// Fonction ecrite apres que de trop grosses regions soient formees sans etre liberes, a cause de la maniere
// dont les transmutations sont calcules.
// Malgre le fait que cette fonction s'execute rarement, elle est utile dans certains cas.
// Le principe est donc que si il existe une region de plus de 10 cases dont le type n'a pas ete recu depuis plus de 5 tours, alors on la transmute.
void decoincer() {
    if(ech_hist.size() < 5)
        return;
    etabli e;
    init_etabli(e, api_moi());
    for(region g : regions(e)) {
        if(g.size() > 10) {
            int type = e[g[0]];
            bool inside = false;
            for(int i = 0 ; i < 5 ; i++) {
                echantillon ech = ech_hist[ech_hist.size()-1-i];
                if(ech.element1 == type or ech.element2 == type) {
                    inside = true;
                    break;
                }
            }
            if(!inside) {
                api_transmuter(to_position(g[0]));
            }
        }
    }
}

/// Fonction appelée à chaque tour.
void jouer_tour()
{   
    cout << "--- tour " << api_tour_actuel() << endl;
    echantillon ech = api_echantillon_tour();
    // Historique d'echantillons..
    ech_hist.push_back(ech);
    
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    // Regarde si l'adversaire est un homme dangereux..
    // 2 conditions possibles pour cela
    // 1 - Il nous a deja attaque
    // 2 - Il possede une grosse zone de mercure/souffre
    // "dangereux" permet d'etre plus efficace et de ne pas jouer defensivement lorsque l'adversaire n'a pas de strategie d'attaque
    if(!dangereux) {
        for(action_hist ah : api_historique()) {
            if(ah.atype == ACTION_CATALYSER) {
                if(ah.id_apprenti == api_moi()) {
                    dangereux = true;
                    break;
                }
            }
        }
        etabli e_enemi;
        init_etabli(e_enemi, api_adversaire());
        for(region r : regions(e_enemi)) {
            if(e_enemi[r[0]] >= SOUFRE) {
                if(r.size() >= 7) {
                    dangereux = true;
                    break;
                }
            }
        }
    }
    
    // Si l'adversaire est dangereux, on a peur.
    if(dangereux) {
        danger_transmut();
    }
    
    // Decoincer le plateau
    decoincer();
    
    // Placage et transmutage d'echantillon
    placer();
    
    // La maniere dont on choisit qui on catalyse est totalement arbitraire, mais apres avoir essaye d'autres methodes
    // comme par exemple de comparer le gain de catalyser sur mon plateau vs le plateau ennemi, cela fonctionanit beaucoup moins bien
    
    if(api_nombre_catalyseurs() >= 2)
        catalyser_pour_attaquer();
    
    for(int i = 0 ; i < api_nombre_catalyseurs()/2 ; i++)
        catalyser_pour_attaquer();
    
    for(int i = 0 ; i < api_nombre_catalyseurs() ; i++)
        catalyser_pour_transmuter(false);
    
    // Dernier tour, vidage du plateau
    if(tour_actuel() >= 149) {
        transmuter_fin();
    }
    
    // Don d'echantillon
    donner2();
    
    // Au cas ou il reste de la catalyse (par exemple apres avoir transmuter au dernier tour)
    for(int i = 0 ; i < api_nombre_catalyseurs() ; i++)
        catalyser_pour_attaquer();
    
    for(int i = 0 ; i < api_nombre_catalyseurs() ; i++)
        catalyser_pour_transmuter(false);
    
    end = std::chrono::system_clock::now();
 
    std::chrono::duration<double> elapsed_seconds = end-start;
    if(elapsed_seconds.count() > 0.02) {
        //std::cout << "WOW! au tour " << api_tour_actuel() << " elapsed time: " << elapsed_seconds.count() << "s\n";
    }
}

/// Fonction appelée à la fin de la partie.
void partie_fin()
{
    
}

