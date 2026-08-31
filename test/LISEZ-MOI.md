# Un banc, faute de banc

La lib n'a pas de cadre de test : elle se vérifie en flashant une carte, ce
qui coûte cher et ne se répète pas.

Ces deux fichiers compilent avec un `c++` ordinaire, en bouchant `Arduino.h`
par quelques lignes. Ils ne remplacent pas un essai sur le matériel — ils
attrapent ce qui n'a pas besoin de matériel pour être faux.

Ce qu'ils ont attrapé en étant écrits :

- `e.text()` là où `text()` appartient à `SignalValue`, pas à `SignalEvent`
- `ISignal` inscrit comme un bloc de GESTE, parce que son enregistrement
  s'écrit exactement comme les cinq autres — un rappel ne lui rendait donc
  plus sa consigne

Les deux compilaient. Aucune relecture ne les aurait vus.

```sh
c++ -std=c++17 -I<racine-du-banc> -w -o t decodeurs.cpp && ./t
```

Le banc lui-même (le faux `Arduino.h` et les copies d'en-têtes) n'est pas
versionné : il se reconstruit en copiant `src/` à côté.
