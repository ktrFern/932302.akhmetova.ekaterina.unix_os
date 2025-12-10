import redis
import json
import time

redis_client = redis.Redis(host='redis', port=6379, db=0)

print("Email worker started. Waiting for tasks...")

while True:
    task_json = redis_client.blpop('email_tasks', timeout=30)

    if task_json:
        task_data = json.loads(task_json[1])
        task_id = task_data['task_id']
        email = task_data['email']
        subject = task_data['subject']
        body = task_data['body']

        print(f"Sending email to {email} (task {task_id})...")
        print(f"Subject: {subject}")
        print(f"Body: {body}")
        time.sleep(2)

        redis_client.setex(
            f'email_status:{task_id}',
            300,
            json.dumps({
                'email': email,
                'status': 'sent',
                'subject': subject
            })
        )

        print(f"Task {task_id}: email sent successfully.")

    time.sleep(0.1)

