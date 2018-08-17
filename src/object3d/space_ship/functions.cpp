/************************************************************************************

	AstroMenace
	Hardcore 3D space scroll-shooter with spaceship upgrade possibilities.
	Copyright (c) 2006-2018 Mikhail Kurinnoi, Viewizard


	AstroMenace is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	AstroMenace is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with AstroMenace. If not, see <https://www.gnu.org/licenses/>.


	Website: https://www.viewizard.com/
	Project: https://github.com/viewizard/astromenace
	E-mail: viewizard@viewizard.com

*************************************************************************************/

// TODO codestyle should be fixed

// TODO translate comments

#include "../object3d.h"
#include "../space_ship/space_ship.h"
#include "../ground_object/ground_object.h"
#include "../projectile/projectile.h"
#include "../space_object/space_object.h"

// NOTE switch to nested namespace definition (namespace A::B::C { ... }) (since C++17)
namespace viewizard {
namespace astromenace {

namespace {

constexpr float RadToDeg = 180.0f / 3.14159f; // convert radian to degree

} // unnamed namespace


// FIXME should be fixed, don't allow global scope interaction for local variables
// направление движения камеры
extern sVECTOR3D GameCameraMovement;
// скорость движения камеры
float GameCameraGetSpeed();


//-----------------------------------------------------------------------------
// Получение угла поворота оружия на врага для космических кораблей
//-----------------------------------------------------------------------------
void GetShipOnTargetOrientation(eObjectStatus ObjectStatus, // статус объекта, который целится
				const sVECTOR3D &Location, // положение точки относительно которой будем наводить
				const sVECTOR3D &CurrentObjectRotation, // текущие углы объекта
				float MinDistance, // минимальное расстояние, с которого начинаем прицеливание
				const float (&RotationMatrix)[9], // матрица вращения объекта
				sVECTOR3D &NeedAngle,// нужные углы, чтобы получить нужное направление
				float Width, // ширина объекта
				bool NeedCenterOrientation, // нужен доворот на центр
				bool NeedByWeaponOrientation, // нужно делать доворот с учетом положения орудия
				const sVECTOR3D &WeponLocation,
				int WeaponType) // тип орудия орудия
{
	// получаем точки для создания плоскости
	sVECTOR3D Orientation{0.0f, 0.0f, 1.0f};
	vw_Matrix33CalcPoint(Orientation, RotationMatrix);
	sVECTOR3D PointUp{0.0f, 1.0f, 0.0f};
	vw_Matrix33CalcPoint(PointUp, RotationMatrix);
	sVECTOR3D PointRight{1.0f, 0.0f, 0.0f};
	vw_Matrix33CalcPoint(PointRight, RotationMatrix);

	// находим плоскость, вертикальную
	float A, B, C, D;
	vw_GetPlaneABCD(A, B, C, D, Location, Location + Orientation, Location + PointUp);

	// получаем вертикальную плоскость 2 (отсечения перед-зад)
	float A2, B2, C2, D2;
	vw_GetPlaneABCD(A2, B2, C2, D2, Location, Location + PointRight, Location + PointUp);

	// для выбора - точка, куда целимся + расстояние до нее (квадрат расстояния)
	sVECTOR3D TargetLocation{Location};
	sVECTOR3D TargetAngle(0.0f, 0.0f, 0.0f);
	float Tdist{1000.0f * 1000.0f};

	// тип, кто заблокировал... чтобы не сбить с активных
	int TType{0};

	bool TargetLocked{false};

	// нам нужна только половина ширины
	float Width2 = Width / 2.0f;

	ForEachSpaceShip([&] (const cSpaceShip &tmpShip) {
		// проверка, чтобы не считать свой корабль
		if ((NeedCheckCollision(tmpShip)) &&
		    ObjectsStatusFoe(ObjectStatus, tmpShip.ObjectStatus)) {
			// находим настоящую точку попадания с учетом скорости объекта и пули... если надо
			sVECTOR3D tmpLocation = tmpShip.GeometryCenter;
			vw_Matrix33CalcPoint(tmpLocation, tmpShip.CurrentRotationMat); // поворачиваем в плоскость объекта
			sVECTOR3D RealLocation = tmpShip.Location + tmpLocation;

			if ((tmpShip.Speed != 0.0f) &&
			    (WeaponType != 0) &&
			    // это не лучевое оружие, которое бьет сразу
			    (WeaponType != 11) && (WeaponType != 12) && (WeaponType != 14) &&
			    // это не ракеты...
			    (WeaponType != 16) && (WeaponType != 17) && (WeaponType != 18) && (WeaponType != 19)) {

				// находим, за какое время снаряд долетит до объекта сейчас
				sVECTOR3D TTT = WeponLocation - RealLocation;
				float ProjectileSpeed = GetProjectileSpeed(WeaponType);
				float CurrentDist = TTT.Length();
				float ObjCurrentTime = CurrentDist / ProjectileSpeed;

				// находим где будет объект, когда пройдет это время
				sVECTOR3D FutureLocation = tmpShip.Orientation ^ (tmpShip.Speed * ObjCurrentTime);

				// находи точку по середине... это нам и надо... туда целимся...
				RealLocation = RealLocation + FutureLocation;
			}

			// проверяем, если с одной стороны все точки - значит мимо, если нет - попали :)
			// + учитываем тут Width
			float tmp1 = A * (RealLocation.x + tmpShip.OBB.Box[0].x) + B * (RealLocation.y + tmpShip.OBB.Box[0].y) + C * (RealLocation.z + tmpShip.OBB.Box[0].z) + D;
			float tmp2 = A * (RealLocation.x + tmpShip.OBB.Box[1].x) + B * (RealLocation.y + tmpShip.OBB.Box[1].y) + C * (RealLocation.z + tmpShip.OBB.Box[1].z) + D;
			float tmp3 = A * (RealLocation.x + tmpShip.OBB.Box[2].x) + B * (RealLocation.y + tmpShip.OBB.Box[2].y) + C * (RealLocation.z + tmpShip.OBB.Box[2].z) + D;
			float tmp4 = A * (RealLocation.x + tmpShip.OBB.Box[3].x) + B * (RealLocation.y + tmpShip.OBB.Box[3].y) + C * (RealLocation.z + tmpShip.OBB.Box[3].z) + D;
			float tmp5 = A * (RealLocation.x + tmpShip.OBB.Box[4].x) + B * (RealLocation.y + tmpShip.OBB.Box[4].y) + C * (RealLocation.z + tmpShip.OBB.Box[4].z) + D;
			float tmp6 = A * (RealLocation.x + tmpShip.OBB.Box[5].x) + B * (RealLocation.y + tmpShip.OBB.Box[5].y) + C * (RealLocation.z + tmpShip.OBB.Box[5].z) + D;
			float tmp7 = A * (RealLocation.x + tmpShip.OBB.Box[6].x) + B * (RealLocation.y + tmpShip.OBB.Box[6].y) + C * (RealLocation.z + tmpShip.OBB.Box[6].z) + D;
			float tmp8 = A * (RealLocation.x + tmpShip.OBB.Box[7].x) + B * (RealLocation.y + tmpShip.OBB.Box[7].y) + C * (RealLocation.z + tmpShip.OBB.Box[7].z) + D;


			if (!(((tmp1 > Width2) && (tmp2 > Width2) && (tmp3 > Width2) && (tmp4 > Width2) &&
			       (tmp5 > Width2) && (tmp6 > Width2) && (tmp7 > Width2) && (tmp8 > Width2)) ||
			      ((tmp1 < -Width2) && (tmp2 < -Width2) && (tmp3 < -Width2) && (tmp4 < -Width2) &&
			       (tmp5 < -Width2) && (tmp6 < -Width2) && (tmp7 < -Width2) && (tmp8 < -Width2)))) {
				// проверяем, спереди или сзади стоит противник
				tmp1 = A2 * (RealLocation.x + tmpShip.OBB.Box[0].x) + B2 * (RealLocation.y + tmpShip.OBB.Box[0].y) + C2 * (RealLocation.z + tmpShip.OBB.Box[0].z) + D2;
				tmp2 = A2 * (RealLocation.x + tmpShip.OBB.Box[1].x) + B2 * (RealLocation.y + tmpShip.OBB.Box[1].y) + C2 * (RealLocation.z + tmpShip.OBB.Box[1].z) + D2;
				tmp3 = A2 * (RealLocation.x + tmpShip.OBB.Box[2].x) + B2 * (RealLocation.y + tmpShip.OBB.Box[2].y) + C2 * (RealLocation.z + tmpShip.OBB.Box[2].z) + D2;
				tmp4 = A2 * (RealLocation.x + tmpShip.OBB.Box[3].x) + B2 * (RealLocation.y + tmpShip.OBB.Box[3].y) + C2 * (RealLocation.z + tmpShip.OBB.Box[3].z) + D2;
				tmp5 = A2 * (RealLocation.x + tmpShip.OBB.Box[4].x) + B2 * (RealLocation.y + tmpShip.OBB.Box[4].y) + C2 * (RealLocation.z + tmpShip.OBB.Box[4].z) + D2;
				tmp6 = A2 * (RealLocation.x + tmpShip.OBB.Box[5].x) + B2 * (RealLocation.y + tmpShip.OBB.Box[5].y) + C2 * (RealLocation.z + tmpShip.OBB.Box[5].z) + D2;
				tmp7 = A2 * (RealLocation.x + tmpShip.OBB.Box[6].x) + B2 * (RealLocation.y + tmpShip.OBB.Box[6].y) + C2 * (RealLocation.z + tmpShip.OBB.Box[6].z) + D2;
				tmp8 = A2 * (RealLocation.x + tmpShip.OBB.Box[7].x) + B2 * (RealLocation.y + tmpShip.OBB.Box[7].y) + C2 * (RealLocation.z + tmpShip.OBB.Box[7].z) + D2;

				if ((tmp1 > 0.0f) && (tmp2 > 0.0f) && (tmp3 > 0.0f) && (tmp4 > 0.0f) &&
				    (tmp5 > 0.0f) && (tmp6 > 0.0f) && (tmp7 > 0.0f) && (tmp8 > 0.0f)) {

					float TargetDist2TMP = A2 * RealLocation.x + B2 * RealLocation.y + C2 * RealLocation.z + D2;

					// проверяем, чтобы объект находился не ближе чем MinDistance
					if (MinDistance < TargetDist2TMP) {
						// выбираем объект, так, чтобы он был наиболее длижайшим,
						// идущим по нашему курсу...

						sVECTOR3D TargetAngleTMP;
						TargetLocation = RealLocation;

						// находим угол между плоскостью и прямой
						float A3, B3, C3, D3;
						vw_GetPlaneABCD(A3, B3, C3, D3, Location, Location + Orientation, Location + PointRight);

						float m = TargetLocation.x - Location.x;
						float n = TargetLocation.y - Location.y;
						float p = TargetLocation.z - Location.z;
						if (NeedByWeaponOrientation) {
							m = TargetLocation.x - WeponLocation.x;
							n = TargetLocation.y - WeponLocation.y;
							p = TargetLocation.z - WeponLocation.z;
						}

						// поправки к существующим углам поворота оружия
						float sss1 = vw_sqrtf(m * m + n * n + p * p);
						float sss2 = vw_sqrtf(A3 * A3 + B3 * B3 + C3 * C3);
						TargetAngleTMP.x = CurrentObjectRotation.x;
						if ((sss1 != 0.0f) && (sss2 != 0.0f)) {
							float sss3 = (A3 * m + B3 * n + C3 * p) / (sss1 * sss2);
							vw_Clamp(sss3, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
							TargetAngleTMP.x = CurrentObjectRotation.x - asinf(sss3) * RadToDeg;
						}

						float sss4 = vw_sqrtf(A * A + B * B + C * C);
						TargetAngleTMP.y = CurrentObjectRotation.y;
						if (NeedCenterOrientation &&
						    (sss1 != 0.0f) && (sss4 != 0.0f)) {
							float sss5 = (A * m + B * n + C * p) / (sss1 * sss4);
							vw_Clamp(sss5, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
							TargetAngleTMP.y = CurrentObjectRotation.y - asinf(sss5) * RadToDeg;
						}

						TargetAngleTMP.z = CurrentObjectRotation.z;

						if ((Tdist > m * m + n * n * 5 + p * p) && (fabsf(TargetAngleTMP.x) < 45.0f)) {
							TargetAngle = TargetAngleTMP;
							Tdist = m * m + n * n * 5 + p * p;
							TargetLocked = true;
							TType = 1;
						}
					}
				}
			}
		}
	});

	// проверка по наземным объектам
	// не стрелять по "мирным" постойкам
	// !!! ВАЖНО
	// у всех наземных объектов ноль на уровне пола...
	ForEachGroundObject([&] (const cGroundObject &tmpGround) {
		// если по этому надо стрелять
		if (NeedCheckCollision(tmpGround) &&
		    ObjectsStatusFoe(ObjectStatus, tmpGround.ObjectStatus)) {

			sVECTOR3D tmpLocation = tmpGround.GeometryCenter;
			vw_Matrix33CalcPoint(tmpLocation, tmpGround.CurrentRotationMat); // поворачиваем в плоскость объекта
			sVECTOR3D RealLocation = tmpGround.Location + tmpLocation;

			if ((tmpGround.Speed != 0.0f) &&
			    (WeaponType != 0) &&
			    // это не лучевое оружие, которое бьет сразу
			    (WeaponType != 11) && (WeaponType != 12) && (WeaponType != 14) &&
			    // это не ракеты...
			    (WeaponType != 16) && (WeaponType != 17) && (WeaponType != 18) && (WeaponType != 19)) {

				// находим, за какое время снаряд долетит до объекта сейчас
				sVECTOR3D TTT = WeponLocation-RealLocation;
				float ProjectileSpeed = GetProjectileSpeed(WeaponType);
				float CurrentDist = TTT.Length();
				float ObjCurrentTime = CurrentDist / ProjectileSpeed;

				// находим где будет объект, когда пройдет это время (+ сразу половину считаем!)
				sVECTOR3D FutureLocation = tmpGround.Orientation ^ (tmpGround.Speed * ObjCurrentTime);

				// находи точку по середине... это нам и надо... туда целимся...
				RealLocation = RealLocation + FutureLocation;
			}

			// проверяем, если с одной стороны все точки - значит мимо, если нет - попали :)
			float tmp1 = A * (tmpGround.Location.x + tmpGround.OBB.Box[0].x) + B * (tmpGround.Location.y + tmpGround.OBB.Box[0].y) + C * (tmpGround.Location.z + tmpGround.OBB.Box[0].z) + D;
			float tmp2 = A * (tmpGround.Location.x + tmpGround.OBB.Box[1].x) + B * (tmpGround.Location.y + tmpGround.OBB.Box[1].y) + C * (tmpGround.Location.z + tmpGround.OBB.Box[1].z) + D;
			float tmp3 = A * (tmpGround.Location.x + tmpGround.OBB.Box[2].x) + B * (tmpGround.Location.y + tmpGround.OBB.Box[2].y) + C * (tmpGround.Location.z + tmpGround.OBB.Box[2].z) + D;
			float tmp4 = A * (tmpGround.Location.x + tmpGround.OBB.Box[3].x) + B * (tmpGround.Location.y + tmpGround.OBB.Box[3].y) + C * (tmpGround.Location.z + tmpGround.OBB.Box[3].z) + D;
			float tmp5 = A * (tmpGround.Location.x + tmpGround.OBB.Box[4].x) + B * (tmpGround.Location.y + tmpGround.OBB.Box[4].y) + C * (tmpGround.Location.z + tmpGround.OBB.Box[4].z) + D;
			float tmp6 = A * (tmpGround.Location.x + tmpGround.OBB.Box[5].x) + B * (tmpGround.Location.y + tmpGround.OBB.Box[5].y) + C * (tmpGround.Location.z + tmpGround.OBB.Box[5].z) + D;
			float tmp7 = A * (tmpGround.Location.x + tmpGround.OBB.Box[6].x) + B * (tmpGround.Location.y + tmpGround.OBB.Box[6].y) + C * (tmpGround.Location.z + tmpGround.OBB.Box[6].z) + D;
			float tmp8 = A * (tmpGround.Location.x + tmpGround.OBB.Box[7].x) + B * (tmpGround.Location.y + tmpGround.OBB.Box[7].y) + C * (tmpGround.Location.z + tmpGround.OBB.Box[7].z) + D;

			if (!(((tmp1 > Width2) && (tmp2 > Width2) && (tmp3 > Width2) && (tmp4 > Width2) &&
			       (tmp5 > Width2) && (tmp6 > Width2) && (tmp7 > Width2) && (tmp8 > Width2)) ||
			      ((tmp1 < -Width2) && (tmp2 < -Width2) && (tmp3 < -Width2) && (tmp4 < -Width2) &&
			       (tmp5 < -Width2) && (tmp6 < -Width2) && (tmp7 < -Width2) && (tmp8 < -Width2)))) {
				// проверяем, спереди или сзади стоит противник
				tmp1 = A2 * (tmpGround.Location.x + tmpGround.OBB.Box[0].x) + B2 * (tmpGround.Location.y + tmpGround.OBB.Box[0].y) + C2 * (tmpGround.Location.z + tmpGround.OBB.Box[0].z) + D2;
				tmp2 = A2 * (tmpGround.Location.x + tmpGround.OBB.Box[1].x) + B2 * (tmpGround.Location.y + tmpGround.OBB.Box[1].y) + C2 * (tmpGround.Location.z + tmpGround.OBB.Box[1].z) + D2;
				tmp3 = A2 * (tmpGround.Location.x + tmpGround.OBB.Box[2].x) + B2 * (tmpGround.Location.y + tmpGround.OBB.Box[2].y) + C2 * (tmpGround.Location.z + tmpGround.OBB.Box[2].z) + D2;
				tmp4 = A2 * (tmpGround.Location.x + tmpGround.OBB.Box[3].x) + B2 * (tmpGround.Location.y + tmpGround.OBB.Box[3].y) + C2 * (tmpGround.Location.z + tmpGround.OBB.Box[3].z) + D2;
				tmp5 = A2 * (tmpGround.Location.x + tmpGround.OBB.Box[4].x) + B2 * (tmpGround.Location.y + tmpGround.OBB.Box[4].y) + C2 * (tmpGround.Location.z + tmpGround.OBB.Box[4].z) + D2;
				tmp6 = A2 * (tmpGround.Location.x + tmpGround.OBB.Box[5].x) + B2 * (tmpGround.Location.y + tmpGround.OBB.Box[5].y) + C2 * (tmpGround.Location.z + tmpGround.OBB.Box[5].z) + D2;
				tmp7 = A2 * (tmpGround.Location.x + tmpGround.OBB.Box[6].x) + B2 * (tmpGround.Location.y + tmpGround.OBB.Box[6].y) + C2 * (tmpGround.Location.z + tmpGround.OBB.Box[6].z) + D2;
				tmp8 = A2 * (tmpGround.Location.x + tmpGround.OBB.Box[7].x) + B2 * (tmpGround.Location.y + tmpGround.OBB.Box[7].y) + C2 * (tmpGround.Location.z + tmpGround.OBB.Box[7].z) + D2;

				if ((tmp1 > 0.0f) && (tmp2 > 0.0f) && (tmp3 > 0.0f) && (tmp4 > 0.0f) &&
				    (tmp5 > 0.0f) && (tmp6 > 0.0f) && (tmp7 > 0.0f) && (tmp8 > 0.0f)) {

					float TargetDist2TMP = A2 * RealLocation.x + B2 * RealLocation.y + C2 * RealLocation.z + D2;

					// проверяем, чтобы объект находился не ближе чем MinDistance
					if (MinDistance < TargetDist2TMP) {
						// выбираем объект, так, чтобы он был наиболее длижайшим,
						// идущим по нашему курсу...
						sVECTOR3D TargetAngleTMP;
						TargetLocation = RealLocation;

						// находим угол между плоскостью и прямой
						float A3, B3, C3, D3;
						vw_GetPlaneABCD(A3, B3, C3, D3, Location, Location + Orientation, Location + PointRight);

						float m = TargetLocation.x - Location.x;
						float n = TargetLocation.y - Location.y;
						float p = TargetLocation.z - Location.z;
						if (NeedByWeaponOrientation) {
							m = TargetLocation.x - WeponLocation.x;
							n = TargetLocation.y - WeponLocation.y;
							p = TargetLocation.z - WeponLocation.z;
						}

						// поправки к существующим углам поворота оружия
						float sss1 = vw_sqrtf(m * m + n * n + p * p);
						float sss2 = vw_sqrtf(A3 * A3 + B3 * B3 + C3 * C3);
						TargetAngleTMP.x = CurrentObjectRotation.x;
						if ((sss1 != 0.0f) && (sss2 != 0.0f)) {
							float sss3 = (A3 * m + B3 * n + C3 * p) / (sss1 * sss2);
							vw_Clamp(sss3, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
							TargetAngleTMP.x = CurrentObjectRotation.x - asinf(sss3) * RadToDeg;
						}

						float sss4 = vw_sqrtf(A * A + B * B + C * C);
						TargetAngleTMP.y = CurrentObjectRotation.y;
						if (NeedCenterOrientation)
							if ((sss1 != 0.0f) && (sss4 != 0.0f)) {
								float sss5 = (A * m + B * n + C * p) / (sss1 * sss4);
								vw_Clamp(sss5, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
								TargetAngleTMP.y = CurrentObjectRotation.y - asinf(sss5) * RadToDeg;
							}

						TargetAngleTMP.z = CurrentObjectRotation.z;

						if ((TType < 2) && TargetLocked) {
							// только если в 5 раза ближе
							if ((Tdist > m * m + n * n * 5 + p * p) && (fabsf(TargetAngleTMP.x) < 45.0f)) {
								TargetAngle = TargetAngleTMP;
								Tdist = m * m + n * n + p * p;
								TargetLocked = true;
								TType = 2;
							}
						} else if ((Tdist > m * m + n * n + p * p) && (fabsf(TargetAngleTMP.x) < 45.0f)) {
							TargetAngle = TargetAngleTMP;
							Tdist = m * m + n * n + p * p;
							TargetLocked = true;
							TType = 2;
						}
					}
				}
			}
		}
	});

	// проверка по космическим объектам
	ForEachSpaceObject([&] (const cSpaceObject &tmpSpace) {
		// если по этому надо стрелять
		if (NeedCheckCollision(tmpSpace) &&
		    ObjectsStatusFoe(ObjectStatus, tmpSpace.ObjectStatus)) {

			sVECTOR3D tmpLocation = tmpSpace.GeometryCenter;
			vw_Matrix33CalcPoint(tmpLocation, tmpSpace.CurrentRotationMat); // поворачиваем в плоскость объекта
			sVECTOR3D RealLocation = tmpSpace.Location + tmpLocation;

			// если нужно проверить
			if ((tmpSpace.Speed != 0.0f) &&
			    (WeaponType != 0) &&
			    // это не лучевое оружие, которое бьет сразу
			    (WeaponType != 11) && (WeaponType != 12) && (WeaponType != 14) &&
			    // это не ракеты...
			    (WeaponType != 16) && (WeaponType != 17) && (WeaponType != 18) && (WeaponType != 19)) {

				// находим, за какое время снаряд долетит до объекта сейчас
				sVECTOR3D TTT = WeponLocation - RealLocation;
				float ProjectileSpeed = GetProjectileSpeed(WeaponType);
				float CurrentDist = TTT.Length();
				float ObjCurrentTime = CurrentDist / ProjectileSpeed;

				// находим где будет объект, когда пройдет это время (+ сразу половину считаем!)
				sVECTOR3D FutureLocation = tmpSpace.Orientation ^ (tmpSpace.Speed * ObjCurrentTime);

				// находи точку по середине... это нам и надо... туда целимся...
				RealLocation = RealLocation + FutureLocation;
			}

			// проверяем, если с одной стороны все точки - значит мимо, если нет - попали :)
			float tmp1 = A * (RealLocation.x + tmpSpace.OBB.Box[0].x) + B * (RealLocation.y + tmpSpace.OBB.Box[0].y) + C * (RealLocation.z + tmpSpace.OBB.Box[0].z) + D;
			float tmp2 = A * (RealLocation.x + tmpSpace.OBB.Box[1].x) + B * (RealLocation.y + tmpSpace.OBB.Box[1].y) + C * (RealLocation.z + tmpSpace.OBB.Box[1].z) + D;
			float tmp3 = A * (RealLocation.x + tmpSpace.OBB.Box[2].x) + B * (RealLocation.y + tmpSpace.OBB.Box[2].y) + C * (RealLocation.z + tmpSpace.OBB.Box[2].z) + D;
			float tmp4 = A * (RealLocation.x + tmpSpace.OBB.Box[3].x) + B * (RealLocation.y + tmpSpace.OBB.Box[3].y) + C * (RealLocation.z + tmpSpace.OBB.Box[3].z) + D;
			float tmp5 = A * (RealLocation.x + tmpSpace.OBB.Box[4].x) + B * (RealLocation.y + tmpSpace.OBB.Box[4].y) + C * (RealLocation.z + tmpSpace.OBB.Box[4].z) + D;
			float tmp6 = A * (RealLocation.x + tmpSpace.OBB.Box[5].x) + B * (RealLocation.y + tmpSpace.OBB.Box[5].y) + C * (RealLocation.z + tmpSpace.OBB.Box[5].z) + D;
			float tmp7 = A * (RealLocation.x + tmpSpace.OBB.Box[6].x) + B * (RealLocation.y + tmpSpace.OBB.Box[6].y) + C * (RealLocation.z + tmpSpace.OBB.Box[6].z) + D;
			float tmp8 = A * (RealLocation.x + tmpSpace.OBB.Box[7].x) + B * (RealLocation.y + tmpSpace.OBB.Box[7].y) + C * (RealLocation.z + tmpSpace.OBB.Box[7].z) + D;

			if (!(((tmp1 > Width2) && (tmp2 > Width2) && (tmp3 > Width2) && (tmp4 > Width2) &&
			       (tmp5 > Width2) && (tmp6 > Width2) && (tmp7 > Width2) && (tmp8 > Width2)) ||
			      ((tmp1 < -Width2) && (tmp2 < -Width2) && (tmp3 < -Width2) && (tmp4 < -Width2) &&
			       (tmp5 < -Width2) && (tmp6 < -Width2) && (tmp7 < -Width2) && (tmp8 < -Width2)))) {
				// проверяем, спереди или сзади стоит противник
				tmp1 = A2 * (RealLocation.x + tmpSpace.OBB.Box[0].x) + B2 * (RealLocation.y + tmpSpace.OBB.Box[0].y) + C2 * (RealLocation.z + tmpSpace.OBB.Box[0].z) + D2;
				tmp2 = A2 * (RealLocation.x + tmpSpace.OBB.Box[1].x) + B2 * (RealLocation.y + tmpSpace.OBB.Box[1].y) + C2 * (RealLocation.z + tmpSpace.OBB.Box[1].z) + D2;
				tmp3 = A2 * (RealLocation.x + tmpSpace.OBB.Box[2].x) + B2 * (RealLocation.y + tmpSpace.OBB.Box[2].y) + C2 * (RealLocation.z + tmpSpace.OBB.Box[2].z) + D2;
				tmp4 = A2 * (RealLocation.x + tmpSpace.OBB.Box[3].x) + B2 * (RealLocation.y + tmpSpace.OBB.Box[3].y) + C2 * (RealLocation.z + tmpSpace.OBB.Box[3].z) + D2;
				tmp5 = A2 * (RealLocation.x + tmpSpace.OBB.Box[4].x) + B2 * (RealLocation.y + tmpSpace.OBB.Box[4].y) + C2 * (RealLocation.z + tmpSpace.OBB.Box[4].z) + D2;
				tmp6 = A2 * (RealLocation.x + tmpSpace.OBB.Box[5].x) + B2 * (RealLocation.y + tmpSpace.OBB.Box[5].y) + C2 * (RealLocation.z + tmpSpace.OBB.Box[5].z) + D2;
				tmp7 = A2 * (RealLocation.x + tmpSpace.OBB.Box[6].x) + B2 * (RealLocation.y + tmpSpace.OBB.Box[6].y) + C2 * (RealLocation.z + tmpSpace.OBB.Box[6].z) + D2;
				tmp8 = A2 * (RealLocation.x + tmpSpace.OBB.Box[7].x) + B2 * (RealLocation.y + tmpSpace.OBB.Box[7].y) + C2 * (RealLocation.z + tmpSpace.OBB.Box[7].z) + D2;

				if ((tmp1 > 0.0f) && (tmp2 > 0.0f) && (tmp3 > 0.0f) && (tmp4 > 0.0f) &&
				    (tmp5 > 0.0f) && (tmp6 > 0.0f) && (tmp7 > 0.0f) && (tmp8 > 0.0f)) {

					float TargetDist2TMP = A2 * RealLocation.x + B2 * RealLocation.y + C2 * RealLocation.z + D2;

					// проверяем, чтобы объект находился не ближе чем MinDistance
					if (MinDistance < TargetDist2TMP) {
						// выбираем объект, так, чтобы он был наиболее длижайшим,
						// идущим по нашему курсу...
						sVECTOR3D TargetAngleTMP;
						TargetLocation = RealLocation;

						// находим угол между плоскостью и прямой
						float A3, B3, C3, D3;
						vw_GetPlaneABCD(A3, B3, C3, D3, Location, Location + Orientation, Location + PointRight);

						float m = TargetLocation.x - Location.x;
						float n = TargetLocation.y - Location.y;
						float p = TargetLocation.z - Location.z;
						if (NeedByWeaponOrientation) {
							m = TargetLocation.x - WeponLocation.x;
							n = TargetLocation.y - WeponLocation.y;
							p = TargetLocation.z - WeponLocation.z;
						}

						// поправки к существующим углам поворота оружия
						float sss1 = vw_sqrtf(m * m + n * n + p * p);
						float sss2 = vw_sqrtf(A3 * A3 + B3 * B3 + C3 * C3);
						TargetAngleTMP.x = CurrentObjectRotation.x;
						if ((sss1 != 0.0f) && (sss2 != 0.0f)) {
							float sss3 = (A3 * m + B3 * n + C3 * p) / (sss1 * sss2);
							vw_Clamp(sss3, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
							TargetAngleTMP.x = CurrentObjectRotation.x - asinf(sss3) * RadToDeg;
						}

						float sss4 = vw_sqrtf(A * A + B * B + C * C);
						TargetAngleTMP.y = CurrentObjectRotation.y;
						if (NeedCenterOrientation &&
						    (sss1 != 0.0f) && (sss4 != 0.0f)) {
							float sss5 = (A * m + B * n + C * p) / (sss1 * sss4);
							vw_Clamp(sss5, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
							TargetAngleTMP.y = CurrentObjectRotation.y - asinf(sss5) * RadToDeg;
						}

						TargetAngleTMP.z = CurrentObjectRotation.z;

						if ((TType < 3) && TargetLocked) {
							// только если в 10 раза ближе
							if ((Tdist / 10.0f > m * m + n * n + p * p) && (fabsf(TargetAngleTMP.x) < 45.0f)) {
								TargetAngle = TargetAngleTMP;
								Tdist = m * m + n * n + p * p;
								TargetLocked = true;
								TType = 3;
							}
						} else if ((Tdist > m * m + n * n + p * p) && (fabsf(TargetAngleTMP.x) < 45.0f)) {
							TargetAngle = TargetAngleTMP;
							Tdist = m * m + n * n + p * p;
							TargetLocked = true;
							TType = 3;
						}
					}
				}
			}
		}
	});

	// находим направление и углы нацеливания на цель, если нужно
	if (TargetLocked)
		NeedAngle = TargetAngle;
}

//-----------------------------------------------------------------------------
// Получение угла поворота оружия на врага для космических кораблей противника
// !! почти полная копия как наведение на у турелей, но без учета выше-ниже противника
// + учет лучевого оружия
//-----------------------------------------------------------------------------
void GetEnemyShipOnTargetOrientation(eObjectStatus ObjectStatus, // статус объекта, который целится
				     const sVECTOR3D &Location, // положение точки относительно которой будем наводить
				     const sVECTOR3D &CurrentObjectRotation, // текущие углы объекта
				     const float (&RotationMatrix)[9], // матрица вращения объекта
				     sVECTOR3D &NeedAngle, // нужные углы, чтобы получить нужное направление
				     int WeaponType) // номер оружия
{
	// получаем точки для создания плоскости
	sVECTOR3D Orientation{0.0f, 0.0f, 1.0f};
	vw_Matrix33CalcPoint(Orientation, RotationMatrix);
	sVECTOR3D PointRight{1.0f, 0.0f, 0.0f};
	vw_Matrix33CalcPoint(PointRight, RotationMatrix);
	sVECTOR3D PointUp{0.0f, 1.0f, 0.0f};
	vw_Matrix33CalcPoint(PointUp, RotationMatrix);

	// находим плоскость, горизонтальную
	float A, B, C, D;
	vw_GetPlaneABCD(A, B, C, D, Location, Location + Orientation, Location + PointRight);


	// для выбора - точка, куда целимся + расстояние до нее (квадрат расстояния)
	sVECTOR3D TargetLocation{Location};
	float TargetDist2{1000.0f * 1000.0f};
	bool TargetLocked{false};

	ForEachSpaceShip([&] (const cSpaceShip &tmpShip) {
		// если по этому надо стрелять
		if (NeedCheckCollision(tmpShip) &&
		    ObjectsStatusFoe(ObjectStatus, tmpShip.ObjectStatus)) {

			sVECTOR3D tmpLocation = tmpShip.GeometryCenter;
			vw_Matrix33CalcPoint(tmpLocation, tmpShip.CurrentRotationMat); // поворачиваем в плоскость объекта
			sVECTOR3D RealLocation = tmpShip.Location + tmpLocation;

			// учитываем, если лазер - наводить не надо
			if (WeaponType != 110) {
				// находим, за какое время снаряд долетит до объекта сейчас
				sVECTOR3D TTT = Location - RealLocation;
				float ProjectileSpeed = GetProjectileSpeed(WeaponType);
				if (ObjectStatus == eObjectStatus::Enemy)
					ProjectileSpeed = ProjectileSpeed / GameEnemyWeaponPenalty;
				float CurrentDist = TTT.Length();
				float ObjCurrentTime = CurrentDist / ProjectileSpeed;

				// находим где будет объект, когда пройдет это время (+ сразу половину считаем!)
				sVECTOR3D FutureLocation = tmpShip.Orientation ^ (tmpShip.Speed * ObjCurrentTime);
				// учитываем камеру...
				sVECTOR3D CamPosTTT(0.0f, 0.0f, 0.0f);
				if (tmpShip.ObjectStatus == eObjectStatus::Player)
					CamPosTTT = GameCameraMovement ^ (GameCameraGetSpeed() * ObjCurrentTime);

				// находи точку по середине... это нам и надо... туда целимся...
				sVECTOR3D PossibleRealLocation = RealLocation + FutureLocation + CamPosTTT;

				TTT = Location - PossibleRealLocation;
				float PossibleDist = TTT.Length();
				float PoprTime = PossibleDist / ProjectileSpeed;

				FutureLocation = tmpShip.Orientation ^ (tmpShip.Speed * PoprTime);
				// учитываем камеру...
				CamPosTTT = sVECTOR3D{0.0f, 0.0f, 0.0f};
				if (tmpShip.ObjectStatus == eObjectStatus::Player)
					CamPosTTT = GameCameraMovement ^ (GameCameraGetSpeed() * PoprTime);

				RealLocation = RealLocation + FutureLocation + CamPosTTT;
			}

			float TargetDist2TMP = (Location.x - RealLocation.x) * (Location.x - RealLocation.x) +
					       (Location.y - RealLocation.y) * (Location.y - RealLocation.y) +
					       (Location.z - RealLocation.z) * (Location.z - RealLocation.z);

			// проверяем, чтобы объект находился не ближе чем MinDistance
			if (TargetDist2 > TargetDist2TMP) {
				// запоминаем эти данные
				TargetLocation = RealLocation;
				TargetDist2 = TargetDist2TMP;
				TargetLocked = true;
			}
		}
	});

	// находим направление и углы нацеливания на цель, если нужно
	if (TargetLocked) {
		// находим угол между плоскостью и прямой
		float m = TargetLocation.x - Location.x;
		float n = TargetLocation.y - Location.y;
		float p = TargetLocation.z - Location.z;

		// поправки к существующим углам поворота оружия
		float sss1 = m * m + n * n + p * p;
		float sss2 = A * A + B * B + C * C;
		if ((sss1 != 0.0f) && (sss2 != 0.0f)) {
			float ttt = (A * m + B * n + C * p) / (vw_sqrtf(sss1) * vw_sqrtf(sss2));
			vw_Clamp(ttt, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
			NeedAngle.x = CurrentObjectRotation.x - asinf(ttt) * RadToDeg;
		}

		NeedAngle.z = CurrentObjectRotation.z;

		// нужно найти точку на плоскости, образованную перпендикуляром с точки TargetLocation
		// иначе не правильно будем ориентировать
		if (sss2 != 0.0f) {
			float t = -(A * TargetLocation.x + B * TargetLocation.y + C * TargetLocation.z + D) / (A * A + B * B + C * C);
			TargetLocation.x = t * A + TargetLocation.x;
			TargetLocation.y = t * B + TargetLocation.y;
			TargetLocation.z = t * C + TargetLocation.z;
			m = TargetLocation.x - Location.x;
			n = TargetLocation.y - Location.y;
			p = TargetLocation.z - Location.z;

			// находим плоскость, вертикальную
			float A2, B2, C2, D2;
			vw_GetPlaneABCD(A2, B2, C2, D2, Location, Location+PointUp, Location+PointRight);

			// смотрим в какой полуплоскости
			float tmp1_1 = A2 * TargetLocation.x + B2 * TargetLocation.y + C2 * TargetLocation.z + D2;
			vw_GetPlaneABCD(A2, B2, C2, D2, Location, Location + Orientation, Location + PointUp);

			if (tmp1_1 >= 0.0f) {
				// находим угол поворота
				sss1 = vw_sqrtf(m * m + n * n + p * p);
				float sss3 = vw_sqrtf(A2 * A2 + B2 * B2 + C2 * C2);
				if ((sss1 != 0.0f) && (sss3 != 0.0f)) {
					float ttt = (A2 * m + B2 * n + C2 * p) / (sss1 * sss3);
					vw_Clamp(ttt, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
					NeedAngle.y = 180.0f - asinf(ttt) * RadToDeg;
				}
			} else {
				// находим угол поворота
				sss1 = vw_sqrtf(m * m + n * n + p * p);
				float sss3 = vw_sqrtf(A2 * A2 + B2 * B2 + C2 * C2);
				if ((sss1 != 0.0f) && (sss3 != 0.0f)) {
					float ttt = (A2 * m + B2 * n + C2 * p) / (sss1 * sss3);
					vw_Clamp(ttt, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
					NeedAngle.y = asinf(ttt) * RadToDeg;
					if (NeedAngle.y < 0.0f)
						NeedAngle.y += 360.0f;
				}
			}
		}
	}
}

} // astromenace namespace
} // viewizard namespace
