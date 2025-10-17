# RaytracingEngine

Petit moteur de ray‑tracing pédagogique en C++20 — rendu basique de sphères et plans avec ombres, normales, profondeur et éclairage ponctuel.

## Fonctionnalités
- Tracé de rayons primaire (depth map).
- Calcul des normales et colormap.
- Éclairage ponctuel (L_i = V(P,Lp) * L_emit / d^2 * Albedo * |N·L|).
- Shadow rays (visibilité) et atténuation physique.
- Export PPM pour visualiser le rendu.

## Arborescence (essentielle)
- `RaytracingEngine/Math.cpp` — vecteurs, rayons, caméra.
- `RaytracingEngine/Shape.h` — Sphere, Plane, HitInfo.
- `RaytracingEngine/Light.h` — définition des lights.
- `RaytracingEngine/Scene.h` — génération depth/normal/color/light maps, combine.
- `RaytracingEngine/RaytracingEngine.cpp` — point d'entrée, initialisation scène.
- `RaytracingEngine/Image.h|cpp` — écriture PPM.

## Prérequis
- Visual Studio 2022 (ou tout compilateur supportant C++20)
- Aucun package externe requis.

## Compilation & exécution (Visual Studio)
1. Ouvrir la solution dans __Solution Explorer__.
2. Sélectionner la configuration (Release/Debug) et la plateforme.
3. Pour lancer sans debugger : __Ctrl+F5__ ou menu __Debug > Start Without Debugging__.
4. Le programme génère `output.ppm` et tente d’ouvrir GIMP via `system("start ...")` — supprimer/adapter si non souhaité.

## Paramètres importants
- Caméra : initialisée dans `RaytracingEngine.cpp` (position, focale, resolution, near/far).
- Lumière : `Light(position, color, intensity)`. `intensity` est un scalaire physique et peut être élevé.
- Planes / Spheres : position, normale, couleur (albédo).

## Comportement de l’éclairage
La formule implémentée est :
L_o = L_e + V(P,L_p) * (L_emit / d^2) * Albedo * max(0, N·L)
- `V(P,L_p)` est évalué par un shadow ray (origine décalée par un `bias`).
- `L_emit` = `light.color * light.intensity`.
- L’atténuation physique utilisée : `1 / d^2`.

## Conseils de debug (problèmes courants)
- Normales mal orientées → surfaces éclairées à l’envers. Vérifier `Plane::normal` et `Sphere::getNormalAt`.
- Self‑intersection / acne : ajuster `bias` (ex. 1e‑3) et rester cohérent avec `HitInfo::hasHitSomething()` (seuil distance).
- Si tout est trop lumineux ou sans contraste : diminuer `ambient`, réduire `light->intensity` ou ajuster atténuation.
- Si vous ne voyez pas d’ombres, tester temporairement `attenuation = 1.0` et `ambient = 0.0` pour isoler la visibilité.
- Multiplications : utiliser `.dot()` pour produit scalaire, `*` pour multiplication composante×scalaire ou composante×composante (couleurs). Le fichier `Math.cpp` contient les opérateurs critiques.

## Extensions suggérées
- Ajouter `Material.emission` et l’ajouter à L_o pour surfaces émissives.
- Soft shadows : échantillonner une light area (multiple shadow rays).
- BRDFs plus réalistes (Cook‑Torrance), textures, réflexions/réfraction (recursion).
- Tone mapping / clamp avant conversion 0..255 dans `tonemap()`.

## Tests rapides
- Mettre `ambient = 0.0` et `attenuation = 1.0` puis vérifier que les ombres nettes apparaissent.
- Dessiner une petite grille ou positionner une sphère en haut à gauche pour valider orientation Y (la camera inverse Y écran → monde).

## Licence & contact
Projet personnel (répertoire cloné : `https://github.com/Sorax5/RaytracingEngine`). Pour questions ou patchs : ouvrir une issue / PR sur le dépôt GitHub.
