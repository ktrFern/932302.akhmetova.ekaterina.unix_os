from flask import Flask, request, jsonify
import redis
import uuid
import json

app = Flask(__name__)
redis_client = redis.Redis(host='redis', port=6379, db=0)

@app.route('/send', methods=['POST'])
def send_email():
    data = request.get_json()

    email = data.get('email')
    subject = data.get('subject')
    body = data.get('body')

    if not email or not subject or not body:
        return jsonify({'error': 'Missing email, subject or body'}), 400

    task_id = str(uuid.uuid4())

    task_data = {
        'task_id': task_id,
        'email': email,
        'subject': subject,
        'body': body
    }

    redis_client.rpush('email_tasks', json.dumps(task_data))

    return jsonify({'task_id': task_id, 'status': 'queued'}), 202


@app.route('/status/<task_id>', methods=['GET'])
def get_status(task_id):
    result = redis_client.get(f'email_status:{task_id}')
    if result:
        return jsonify({'task_id': task_id, 'status': json.loads(result)})
    return jsonify({'task_id': task_id, 'status': 'processing'}), 202


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

