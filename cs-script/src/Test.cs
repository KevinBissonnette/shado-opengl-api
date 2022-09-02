﻿using System;
using System.Collections.Generic;
using System.Threading;
using Shado;
using Shado.math;

namespace Sandbox
{   
   
    public class Test : Entity
    {
        private SpriteRendererComponent sprite; 
        private Entity camera;
        private List<Entity> entities = new List<Entity>();
        private bool flag = false;

        protected override void OnCreate() {
            camera = Scene.GetPrimaryCameraEntity();

            Debug.Warn("Transform {0}", Transform.Position);

            AddComponent<SpriteRendererComponent>();
            AddComponent<RigidBody2DComponent>().Type = RigidBody2DComponent.BodyType.DYNAMIC;
            var bcparent = AddComponent<BoxCollider2DComponent>();
            bcparent.Restitution = 0.7f;
            bcparent.RestitutionThreshold = 0.6f;

            sprite = GetComponent<SpriteRendererComponent>();
            sprite.Color = new Vector4(0.3f, 1.0f, 0.7f, 1.0f);

            Texture2D texture = new Texture2D(@"assets\riven.png");
            for (int i = 1; i < 4; i++) { 
                Entity entity = Entity.Create();
                entity.Transform.Position = Transform.Position + new Vector3(i * 2, 0.0f, 0.0f);
                entity.Transform.Rotation = Transform.Rotation;
                entity.Tag = "hehexd " + i;

                var sprite = entity.AddComponent<CircleRendererComponent>();
                sprite.Texture = texture;


                var rb = entity.AddComponent<RigidBody2DComponent>();
                rb.Type = RigidBody2DComponent.BodyType.DYNAMIC;

                var bc = entity.AddComponent<CircleCollider2DComponent>();
                bc.Restitution = 0.7f;
 
                entities.Add(entity);
            }

            //DestroyAfter(3);
        }

        protected override void OnUpdate(float dt)
        {
            if (!flag && Input.IsKeyPressed(KeyCode.Space)) {
                Debug.Info("hehexd");
                entities[0].Destroy();
                entities.RemoveAt(0);
                flag = true;
            }

            if (camera != null)
            {
                if (Input.IsKeyPressed(KeyCode.D))
                {
                    camera.Transform.Position += new Vector3(0.05f, 0.0f, 0.0f);
                }
                else if (Input.IsKeyPressed(KeyCode.A))
                {
                    camera.Transform.Position -= new Vector3(0.05f, 0.0f, 0.0f);
                }

                if (Input.IsKeyPressed(KeyCode.W))
                {
                    camera.Transform.Position += new Vector3(0.0f, 0.05f, 0.0f);
                }
                else if (Input.IsKeyPressed(KeyCode.S))
                {
                    camera.Transform.Position -= new Vector3(0.0f, 0.05f, 0.0f);
                }
            }

            var pos = Transform.Position;
            pos.Normalize();
            sprite.Color = new Vector4(
                Math.Abs(pos.x), Math.Abs(pos.y), Math.Abs(pos.z), 1.0f
            );
            
        }

        protected override void OnDestroyed()
        {
            Debug.Info("{0} destroyed!", Id);
        }

        protected override void OnCollision2DEnter(Collision2DInfo info, Entity other)
        {
            // var sprite = GetComponent<SpriteRendererComponent>();
            //sprite.Color = Color.White;
            //Destroy();
        }

        protected override void OnCollision2DLeave(Collision2DInfo info, Entity other)
        {
            //var sprite = GetComponent<SpriteRendererComponent>();
            //sprite.Color = Vector4.One;
        }


        private void DestroyAfter(int seconds) {

            ThreadManager.Create(() => { 
                Thread.Sleep(seconds * 1000);
                this.Destroy();
            }).Start();
        }

    }
}