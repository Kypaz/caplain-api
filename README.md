## Usage

```
git clone --recursive https://github.com/khumbunix/caplain-api.git
cd caplain-api/tidy-html5/build/cmake/
cmake ../..
make
sudo make install
```

## BUG

# Print all <I> tags : some unwanted

# Bug with some "donnees" DIV containing <A tags> as this one
```
<DIV CLASS="donnees">
pression 1020 mb
<A CLASS="info" HREF="#">en plaine<SPAN>bassin grenoblois</SPAN></A>: 17/22°C, vent faible->NNW/5 <A CLASS="info" HREF="#">noeuds<SPAN>1 noeud = 1.852 km/h = 0.51 m/s</SPAN></A>
à 1500m hors-sol: 13°C, vent NW/6->Nord/9 noeuds
à 3000m hors-sol: 5°C, vent NW->Nord/20-25 noeuds, +fort à l'Ouest
iso 0°: 3900->4400m
</DIV>

# No carriage return for donnees div, encoding messing up ?
