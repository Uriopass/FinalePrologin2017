# Prologin2017 

[Sujet de la finale](https://prologin.org/static/archives/2017/finale/sujet/tabulaprologina.pdf)

Le code C++ est contenu dans « prologin.cc »

Pour tester si les modifications apportées était des regressions ou non, je faisais un tournoi maison avec l’ancien vs le nouveau. Ce code est dans tournoi.py.

Aucune traduction vers l’anglais n’est prévue pour l’instant.

A noter que le code n’est pas testable, car inclure toutes les dépendances serait trop fastidieux, ce repo n’est donc que pour les curieux qui se posent des questions sur la stratégie utilisée par le vainqueur.

# RENDU

## Strategie de ce bot :
On coupe en deux la strategie de don d'echantillon et de placement/transmutation/catalyse.

## Preliminaire :
  Un concept tres important pour cette IA est la notion de _score de plateau/etabli_
  Le principe est de donner un score a un etabli sans prendre en compte le reste du contexte (echantillon recu, echantillon donne, etc..)
  Un grand temps a ete passe a paufiner cette fonction et a ajouter pleins de petites informations.
  De telles informations inclues : 
  - Le nombre d'or/catalyse rapportee si on vidait le plateau
  - Le nombre de "regions", cad de blocs adjaceant de meme type (on enleve une region lorsqu'on transmutte)
  - Le nombre de trou, cad de regions vides isolees
  - De combien les regions sont etires sur le plateau (comme des serpents)
  - Le nombre de cases vide a cote de chaque region (isolement)
  
## Strategie de placement : 
  Le principe (et comme pour toutes les autres strategie) est de raisonner en fonction du score de plateau.
  Ainsi pour le placement, on itere toutes les possibilitees de placement d'echantillon et on regarde celle qui donne le meilleur score
  A noter qu'on itere aussi les placements qui incluent une transmutation au prealable. (avec un malus pour ceux la)

## Strategie de catalyse (defense) :
  Idem que pour le placement, on itere toutes les manieres de catalyser, et on prend la meilleur.

## Strategie de catalyse (attaque) :
  Comme pour la strategie de defense, simplement on prend celle qui fait perdre le plus de points a l'adversaire

## Strategie de transmutation : 
  Il n'y a en fait pas a proprement parler de strategie de transmutation, cellent ci sont automatiquement effectuees dans 
  la strategie de placement (a partir d'une certaine taille, detruire une region n'est plus un malus)
  
  En revanche, il existe un autre cas ou une transmutation va s'effectuer (decrite par la fonction decoincer)
  Le principe etant que si on possede une grande (>10) region dont l'adversaire ne nous as pas donne le type dans ses echantillons depuis plus de 5 tours, 
  alors on suppose qu'on ne pourra plus la faire grandir, et ainsi on la transmute.
  Cette technique date du debut du concours, mais l'enlever faisait perdre l'IA dans certains cas.

Toutes ces strategies sont decrites un peu plus en detail dans la documentation des fonctions.

## Strategie de don : 
  La strategie de don se rapproche aussi beaucoup du reste.
  On itere parmis touts les echantillons donnables (10), puis on se mets a la place de l'adversaire
  en essayant de placer cette echantillon de la meilleur maniere possible.
  On choisit ainsi l'echantillon qui a mis l'adversaire dans le plus de difficulte.

## Strategie de defense par la fuite :
  Tout d'abord, il sagit d'identifier si l'ennemi est offensif ou non.
  Un enemi est considere offensif si il catalyse au moins une fois le plateau allie, ou qu'il construit 
  une tres grande region de mercure/soufre.
  
  Une fois que l'ennemi est identifie comme dangereux, alors on effectue la strategie de defense:
  Le principe est que si l'on detecte qu'au tour suivant il pourrait nous detruire notre magnifique region metalique qui a un fort potentiel, 
  alors on transmute cette zone a risque, afin de recuperer tout l'or possible.
  Bien sur, cela est tres difficile a equilibrer, car si l'on est trop sensible alors on ne profitera pas de la quadracite de l'or de regions solide.
  En revanche, si l'on est pas assez sensible, alors on risque d'etre trop ambitieux tout cela pour se faire marcher dessus par les catalyseurs ennemis.

## Si j'avais eu plus de temps :
 Si j'avais eu plus de temps, j'aurai ajoute une feature auquel j'avais pense au debut du concours.
 Pour l'instant, l'ordre est toujours le meme :
 
  - si dangereux:
      se proteger
  - se decoincer
  - placer
  - utiliser cataliseurs
  - transmuter le plateau (si c'est le dernier tour)
  - determiner quoi donner
  - catalyser ce qu'il reste
  
  Ainsi par exemple, une chose que j'ai vu chez d'autres bots mais que je n'aurais pas pu faire a cause de la structure est :
  - transmuter
  - catalyser
  - placer
  J'aurais donc essaye d'explorer d'autres ordres (il n'y en as pas tant que ca qui sont intelligents), et prendre l'ordre le plus adequat

  Un autre feature aurait ete d'unifier l'attaque et la defense de la catalyse, car pour l'instant l'ordre est relativement arbitraire.
  Il se trouve que j'ai essaye d'implementer cette feature mais elle ne donnait pas de bons resultats a cause de la non-linearite de l'algorithme de score de plateau 
  (cela empeche de comparer des gains)

## PS:
  Une grande decouverte cette annee qui a grandement aide a la determination des coefficients et algorithme 
  est le principe de faire jouer le bot contre lui meme (a l'aide d'un script, sur environ 100-200 parties a chaque fois). 
  Ainsi, a chaque fois qu'une fonctionnalite etait implemente, je verifiais qu'il n'y avais eu aucune regression en faisant 
  se battre la nouvelle IA contre l'IA precedente.
  Cela a notamment permis d'eliminer le mix des deux catalyse (catalyse defensive et offensive) qui auraient
  mene ce bot a sa perte.

  90% des coeffs ont ainsi naturellement ete determine en utilisant cette technique.
